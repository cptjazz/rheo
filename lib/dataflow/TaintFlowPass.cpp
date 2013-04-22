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

void TaintFlowPass::enqueueFunctionsInCorrectOrder(const CallGraphNode* node, set<const Function*>& circleHelper) {
  Function* f = node->getFunction();

  for (CallGraphNode::const_iterator i = node->begin(), e = node->end(); i != e; ++i) { 
    const CallGraphNode* kid = i->second;
    const Function* kf = kid->getFunction();
    // Skip dummy nodes
    if (!kf)
      continue;

    // Skip already processed functions
    if (circleHelper.count(kf)) {
      continue;
    } else {
      circleHelper.insert(kf);
    }

    NodeVector& circle = _circularReferences[kf];
    for (vector<const CallGraphNode*>::iterator f_i = circle.begin(), f_e = circle.end(); f_i != f_e; f_i++) {
      const CallGraphNode* circleElem = *f_i;

      if (circleElem->getFunction())
        enqueueFunctionsInCorrectOrder(circleElem, circleHelper);
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

bool TaintFlowPass::buildCircularReferenceInfoRecursion(const CallGraphNode* node, const CallGraphNode* startNode, NodeVector& circularReferences) {
  for (CallGraphNode::const_iterator i = node->begin(), e = node->end(); i != e; ++i) { 
    const CallGraphNode* kid = i->second;

    if (kid == startNode) {
      // Found circle.
      circularReferences.push_back(node);
      return true;
    }

    Function* f = kid->getFunction();
    if (_avoidInfiniteLoopHelper.count(f))
      // Already processed
      continue;

    _avoidInfiniteLoopHelper.insert(f);

    if (buildCircularReferenceInfoRecursion(kid, startNode, circularReferences)) {
      circularReferences.push_back(node);
      return true;
    }
  }

  return false;
}

inline void TaintFlowPass::buildCircularReferenceInfo(CallGraph& CG) {
  for (CallGraph::const_iterator i = CG.begin(), e = CG.end(); i != e; ++i) { 
    _avoidInfiniteLoopHelper.clear();
    const CallGraphNode* n = i->second;

    NodeVector refList;
    _circularReferences.insert(make_pair(n->getFunction(), refList));
    buildCircularReferenceInfoRecursion(n, n, _circularReferences[n->getFunction()]);
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
    _occurrenceCount.insert(make_pair(f, 1));
  }

  buildCircularReferenceInfo(CG);


  for (CircleMap::const_iterator f_i = _circularReferences.begin(), f_e = _circularReferences.end(); f_i != f_e; ++f_i) {
    const NodeVector& list = _circularReferences[f_i->first];

    if (!f_i->first)
      continue;

    if (! list.size())
      continue;

    DEBUG(errs() << "Found circ-ref: " << f_i->first->getName() << " --> ");
    for (NodeVector::const_iterator c_i = list.begin(), c_e = list.end(); c_i != c_e; c_i++) { 
      DEBUG(errs() << "  " << (*c_i)->getFunction()->getName());
    }
    DEBUG(errs() << "\n");
  }

  _queuedFunctionHelper.clear();
  _avoidInfiniteLoopHelper.clear();
  set<const Function*> circleHelper;
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
  ProcessingState state = runOnFunction(func, result);

  switch (state) {
    case Deferred:
      errs() << "__defer:" << func.getName() << "\n";
      _functionQueue.push(&func);
      break;

    case Success:
      TaintFile::writeResult(func, result);
      break;

    default:
      errs() << "Skipping `" << func.getName() << "`\n";
      break;
  }
}

ProcessingState TaintFlowPass::runOnFunction(Function& func, ResultSet& result) {
  long time = Helper::getTimestamp();

  FunctionProcessor proc(*this, func, _circularReferences, *func.getParent(), result, errs());
  proc.processFunction();

  time = Helper::getTimestampDelta(time);

  errs() << "__logtime:" << func.getName() << ":" << time << " µs\n";

  return proc.getState();
}
