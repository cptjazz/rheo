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

static RegisterPass<TaintFlowPass> X("dataflow", "Taint-flow analysis", false, false);

void TaintFlowPass::enqueueFunctionsInCorrectOrder(const CallGraphNode* node, set<const Function*>& circleHelper) {
  Function* f = node->getFunction();
  set<const CallGraphNode*> deferred;
  
  // Skip already processed functions
  if (circleHelper.count(f)) {
    return;
  } else {
    circleHelper.insert(f);
  }

  for (CallGraphNode::const_iterator i = node->begin(), e = node->end(); i != e; ++i) { 
    const CallGraphNode* kid = i->second;
    const Function* kf = kid->getFunction();

    // Skip dummy nodes
    if (!kf)
      continue;

    NodeVector& circle = _circularReferences[kf];
    if (circle.size()) {
      deferred.insert(kid);
      continue;
    }

    enqueueFunctionsInCorrectOrder(kid, circleHelper);
  }

  for (set<const CallGraphNode*>::iterator d_i = deferred.begin(), d_e = deferred.end(); d_i != d_e; ++d_i) {
    const CallGraphNode* kid = *d_i;
    const Function* kf = kid->getFunction();

    // Skip dummy nodes
    if (!kf)
      continue;

    NodeVector& circle = _circularReferences[kf];
    for (NodeVector::iterator c_i = circle.begin(), c_e = circle.end(); c_i != c_e; ++c_i) {
      enqueueFunctionsInCorrectOrder(kid, circleHelper);
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

    if (!f->isIntrinsic() && !TaintFile::exists(*f)) {
      errs() << "__external:" << f->getName() << "\n";
    }
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
    _functionQueue.push_back(f);
    _queuedFunctionHelper.insert(f);
    DEBUG(errs() << "Enqueued: " << f->getName() << "\n");
  }
}

bool TaintFlowPass::runOnModule(Module &module) {
  CallGraph& CG = getAnalysis<CallGraph>();

  errs() << "__enqueue:start\n";
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

  errs() << "__enqueue:end\n";

  while (!_functionQueue.empty()) {
    const Function* f = _functionQueue.front();

    // Skip if function was already processed.
    if (TaintFile::exists(*f)) {
      _functionQueue.pop_front();
      continue;
    }

    if (_occurrenceCount[f]++ > 3) {
      errs() << "__error:PANIC: detected endless loop. Aborting.\n";
      return false;
    }

    ProcessingState state = processFunction(*f, module);
    _functionQueue.pop_front();

    // If the current function was deferred,
    // there is no need to check the depending functions,
    // because they would be deferred again.
    if (state == Deferred)
      continue;

    // Enqueue deferred functions that depend on the currently processed function
    // at the front of the function queue.
    multimap<const Function*, const Function*>::const_iterator d_i = _deferredFunctions.lower_bound(f);
    multimap<const Function*, const Function*>::const_iterator d_e = _deferredFunctions.upper_bound(f);
    for (; d_i != d_e; d_i++) {
      DEBUG(errs() << "pushing deferred to front:" << d_i->second->getName() << "\n");
      _occurrenceCount[d_i->second]--;
      _functionQueue.push_front(d_i->second);
    }

    _deferredFunctions.erase(f);
  }

  return false;
}

ProcessingState TaintFlowPass::processFunction(const Function& func, const Module& module) {
  errs() << "# Run per function pass on `" << func.getName() << "`\n";
  errs() << "__log:start:" << func.getName() << "\n";

  long time = Helper::getTimestamp();

  FunctionProcessor proc(*this, func, _circularReferences, module, logger);
  proc.processFunction();
  ResultSet result = proc.getResult();

  time = Helper::getTimestampDelta(time);

  errs() << "__logtime:" << func.getName() << ":" << time << " µs\n";
  AnalysisState analysisState = proc.getAnalysisState();
  ProcessingState state = analysisState.getProcessingState();
  const Function* missing = analysisState.getMissingDefinition();

  switch (state) {
    case Deferred:
      if (missing) {
        errs() << "__defer:" << func.getName();
        if (missing->hasName())
          errs()  << ":" << missing->getName();
        errs() << "\n";

        _deferredFunctions.insert(make_pair(missing, &func));
      } else {
        // print error!?
        errs() << "__error:Could not evaluate dependency.\n";
      }

      break;

    case Success:
      TaintFile::writeResult(func, result);
      break;

    default:
      errs() << "Skipping `" << func.getName() << "`\n";
      break;
  }

  return state;
}
