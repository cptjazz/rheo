#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/InstrTypes.h"
#include <queue>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "TaintFlowPass.h"
#include "GraphExporter.h"
#include "FunctionProcessor.h"
#include "TaintFile.h"


char TaintFlowPass::ID = 0;

static RegisterPass<TaintFlowPass> X("dataflow", "Taint-flow analysis", true, true);

void TaintFlowPass::enqueueFunctionsInCorrectOrder(const CallGraphNode* node, FunctionMap& circleHelper) {
  Function* f = node->getFunction();

  for (CallGraphNode::const_iterator i = node->begin(), e = node->end(); i != e; ++i) { 
    const CallGraphNode* kid = i->second;

    // Skip dummy nodes
    if (!kid->getFunction())
      continue;

    // Skip already processed functions
    if (f) {
      if (circleHelper.count(make_pair(f, kid->getFunction()))) {
        continue;
      } else {
        circleHelper.insert(make_pair(f, kid->getFunction()));
      }
    }

    enqueueFunctionsInCorrectOrder(kid, circleHelper);
  }

  if (!f)
    return;

  // Skip external (library) functions
  if (f->size()) {
    addFunctionForProcessing(f);
  } else {
    DEBUG(errs() << "Skip enqueue (external): " << f->getName() << "\n");
  }
}

void TaintFlowPass::buildCircularReferenceInfo(const CallGraphNode* node, const CallGraphNode* startNode) {
  for (CallGraphNode::const_iterator i = node->begin(), e = node->end(); i != e; ++i) { 
    const CallGraphNode* kid = i->second;

    if (kid == startNode) {
      // Found circle.
      _circularReferences.insert(make_pair(node->getFunction(), startNode->getFunction()));
      continue;
    }

    Function* f = kid->getFunction();
    if (_avoidInfiniteLoopHelper.count(f))
      // Already processed
      continue;

    _avoidInfiniteLoopHelper.insert(f);
    buildCircularReferenceInfo(kid, startNode);
  }
}

void TaintFlowPass::addFunctionForProcessing(Function* f) {
  if (!_queuedFunctionHelper.count(f)) {
    _functionQueue.push(f);
    _queuedFunctionHelper.insert(f);
    DEBUG(errs() << "Enqueued: " << f->getName() << "\n");
  }
}

bool TaintFlowPass::runOnModule(Module &module) {
  CallGraph& CG = getAnalysis<CallGraph>();

  _avoidInfiniteLoopHelper.clear();
  for (Module::iterator i = module.begin(), e = module.end(); i != e; ++i) {
    Function* f = &*i;
    _occurrenceCount.insert(pair<Function*, int>(f, 1));
  }

  for (CallGraph::const_iterator i = CG.begin(), e = CG.end(); i != e; ++i) { 
    _avoidInfiniteLoopHelper.clear();
    const CallGraphNode* n = i->second;
    buildCircularReferenceInfo(n, n);
  }

  for (FunctionMap::const_iterator f_i = _circularReferences.begin(), f_e = _circularReferences.end(); f_i != f_e; ++f_i) {
    DEBUG(errs() << "Found circ-ref: " << f_i->first->getName() << " <--> " << f_i->second->getName() << "\n");
  }

  _queuedFunctionHelper.clear();
  _avoidInfiniteLoopHelper.clear();
  FunctionMap circleHelper;
  enqueueFunctionsInCorrectOrder(CG.getRoot(), circleHelper);

  while (!_functionQueue.empty()) {
    Function* f = _functionQueue.front();

    // Skip if function was already processed.
    if (TaintFile::exists(*f)) {
      _functionQueue.pop();
      continue;
    }

    if (_occurrenceCount[f]++ > 3) {
      errs() << "__error:PANIC: detected endless loop. Aborting.\n";
      return false;
    }

    processFunction(*f);
    _functionQueue.pop();
  }

  return false;
}

void TaintFlowPass::processFunction(Function& func) {
  errs() << "# Run per function pass on `" << func.getName() << "`\n";
  errs() << "__log:start:" << func.getName() << "\n";

  ResultSet result;
  bool state = runOnFunction(func, result);

  if (!state) {
    errs() << "Skipping `" << func.getName() << "`\n";
    return;
  }

  TaintFile::writeResult(func, result);
}

bool TaintFlowPass::runOnFunction(Function& func, ResultSet& result) {
  long time = Helper::getTimestamp();

  FunctionProcessor proc(*this, func, _circularReferences, *func.getParent(), result, errs());
  proc.processFunction();
  bool finished = proc.didFinish();

  time = Helper::getTimestampDelta(time);

  errs() << "__logtime:" << func.getName() << ":" << time << " µs\n";

  return finished;
}
