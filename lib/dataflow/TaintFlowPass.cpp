#include "TaintFlowPass.h"
#include "TaintFile.h"
#include "RequestsFile.h"
#include "FunctionProcessor.h"
#include "SpecialTaintHelper.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/Casting.h"
#include <queue>
#include <algorithm>
#include <string>


char TaintFlowPass::ID = 0;
static RegisterPass<TaintFlowPass> X("dataflow", "Taint-flow analysis", false, false);


void TaintFlowPass::enqueueFunctionsInCorrectOrder(const CallGraphNode* node, std::set<const Function*>& circleHelper) {
  Function* f = node->getFunction();
  std::set<const CallGraphNode*> deferred;
  
  // Skip already processed functions
  if (circleHelper.count(f)) {
    return;
  } else {
    circleHelper.insert(f);
    if (f)
      DOT->addCGFunction(*f);
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

    if (f)
      DOT->addCGCall(*f, *kf);
    enqueueFunctionsInCorrectOrder(kid, circleHelper);
  }

  for (std::set<const CallGraphNode*>::iterator d_i = deferred.begin(), d_e = deferred.end(); d_i != d_e; ++d_i) {
    const CallGraphNode* kid = *d_i;
    const Function* kf = kid->getFunction();

    // Skip dummy nodes
    if (!kf)
      continue;

    if (f)
      DOT->addCGCall(*f, *kf);
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
    _circularReferences.insert(std::make_pair(n->getFunction(), refList));
    buildCircularReferenceInfoRecursion(n, n, _circularReferences[n->getFunction()]);
  }
}

void TaintFlowPass::addFunctionForProcessing(Function* f) {
  if (!_queuedFunctionHelper.count(f)) {
    if (TaintFile::exists(*f))
      return;

    _functionQueue.push_back(f);
    _queuedFunctionHelper.insert(f);
    DEBUG(errs() << "Enqueued: " << f->getName() << "\n");
  }
}

bool TaintFlowPass::runOnModule(Module &module) {
  CallGraph& CG = getAnalysis<CallGraph>();
  DOT = new GraphExporter("callgraph");
  DOT->init();

  // Handle exclusion list
  ExcludeFile& excludes = ExcludeFile::read();
  for (CallGraph::iterator i = CG.begin(), e = CG.end(); i != e; ++i) {
    CallGraphNode* node = i->second;
    Function& f = *node->getFunction();

    // We delete the function body to
    // "convert" this function to external
    // and let the external heuristic do
    // its job.
    if (excludes.includesFunction(&f)) {
      f.deleteBody();
      continue;
    }
  }

  errs() << "__enqueue:start\n";
  _avoidInfiniteLoopHelper.clear();
  for (Module::iterator i = module.begin(), e = module.end(); i != e; ++i) {
    Function* f = &*i;
    _occurrenceCount.insert(std::make_pair(f, 1));
  }

  buildCircularReferenceInfo(CG);
  DEBUG(printCircularReferences());

  _queuedFunctionHelper.clear();
  _avoidInfiniteLoopHelper.clear();
  std::set<const Function*> circleHelper;

  RequestsFile& requests = RequestsFile::read();

  for (CallGraph::iterator i = CG.begin(), e = CG.end(); i != e; ++i) {
    CallGraphNode* node = i->second;
    Function& f = *node->getFunction();

    if (requests.includesFunction(&f))
      enqueueFunctionsInCorrectOrder(node, circleHelper);
  }

  errs() << "__enqueue:end\n";
  errs() << "__enqueue:count:" << _functionQueue.size() << "\n";

  delete (DOT);

  processFunctionQueue(module, excludes);

  return false;
}

void TaintFlowPass::processFunctionQueue(const Module& module, ExcludeFile& exclusions) {
  while (!_functionQueue.empty()) {
    const Function* f = _functionQueue.front();

    if (_occurrenceCount[f]++ > 3) {
      errs() << "__error:PANIC: detected endless loop. Aborting.\n";
      return;
    }

    ProcessingState state = processFunction(*f, module, exclusions);
    _functionQueue.pop_front();

    // If the current function was deferred,
    // there is no need to check the depending functions,
    // because they would be deferred again.
    if (state == Deferred)
      continue;

    // Enqueue deferred functions that depend on the currently processed function
    // at the front of the function queue.
    std::multimap<const Function*, const Function*>::const_iterator d_i = _deferredFunctions.lower_bound(f);
    std::multimap<const Function*, const Function*>::const_iterator d_e = _deferredFunctions.upper_bound(f);
    for (; d_i != d_e; d_i++) {
      DEBUG(errs() << "pushing deferred to front:" << d_i->second->getName() << "\n");
      _occurrenceCount[d_i->second]--;
      _functionQueue.push_front(d_i->second);
    }

    _deferredFunctions.erase(f);
  }
}

void TaintFlowPass::printCircularReferences() {
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

}

ProcessingState TaintFlowPass::processFunction(const Function& func, const Module& module, ExcludeFile& exclusions) {
  errs() << "# Run per function pass on `" << func.getName() << "`\n";
  errs() << "__log:start:" << func.getName() << "\n";

  SpecialTaintHelper sth(module.getContext(), logger);
  FunctionProcessor proc(*this, func, _circularReferences, module, logger, exclusions, sth);
  proc.processFunction();
  ResultSet result = proc.getResult();

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

        _deferredFunctions.insert(std::make_pair(missing, &func));
      } else {
        // print error!?
        errs() << "__error:Could not evaluate dependency.\n";
      }

      break;

    case Success:
      TaintFile::writeTempResult(sth, func, result);
      TaintFile::persistResult(func);
      break;

    default:
      errs() << "Skipping `" << func.getName() << "`\n";
      break;
  }

  return state;
}
