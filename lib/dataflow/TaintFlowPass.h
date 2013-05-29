#ifndef TAINTFLOWPASS_H
#define TAINTFLOWPASS_H

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include "Core.h"
#include "AnalysisState.h"
#include "Logger.h"
#include <queue>

using namespace llvm;
using namespace std;

  const unsigned int OUTPUT_RELEASE = 0;

  class TaintFlowPass : public ModulePass {

    multimap<const Function*, const Function*> _deferredFunctions;
    deque<const Function*> _functionQueue;
    map<const Function*, int> _occurrenceCount;
    CircleMap _circularReferences;
    set<Function*> _queuedFunctionHelper;
    set<Function*> _avoidInfiniteLoopHelper;
    Logger logger;


    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<CallGraph>();
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

    bool runOnModule(Module &module); 
    void enqueueFunctionsInCorrectOrder(const CallGraphNode* node, set<const Function*>& circleHelper); 
    void buildCircularReferenceInfo(CallGraph& CG);
    bool buildCircularReferenceInfoRecursion(const CallGraphNode* node, const CallGraphNode* startNode, NodeVector& circularReferences);
    void addFunctionForProcessing(Function* f); 
    ProcessingState processFunction(const Function& func);

public:
    static char ID;

    TaintFlowPass() : ModulePass(ID), logger(errs(), nulls()) { }

    template<class TDep>
    TDep& getDependency(const Function& f) {
      return getAnalysis<TDep>(const_cast<Function&>(f));
    }

  };


#endif // TAINTFLOWPASS_H
