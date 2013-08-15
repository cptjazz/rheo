#ifndef TAINTFLOWPASS_H
#define TAINTFLOWPASS_H

#include "Core.h"
#include "AnalysisState.h"
#include "Logger.h"
#include "GraphExporter.h"
#include "ExcludeFile.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CallGraph.h"
#include <queue>



class TaintFlowPass : public ModulePass {
  private:

    std::multimap<const Function*, const Function*> _deferredFunctions;
    std::deque<const Function*> _functionQueue;
    std::map<const Function*, int> _occurrenceCount;

    CircleMap _circularReferences;
    std::set<Function*> _queuedFunctionHelper;
    std::set<Function*> _avoidInfiniteLoopHelper;
    Logger logger;
    GraphExporter* DOT;

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<CallGraph>();
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
      AU.setPreservesAll();
    }

    bool runOnModule(Module &module); 
    void enqueueFunctionsInCorrectOrder(const CallGraphNode* node, std::set<const Function*>& circleHelper); 
    void buildCircularReferenceInfo(CallGraph& CG);
    bool buildCircularReferenceInfoRecursion(const CallGraphNode* node, const CallGraphNode* startNode, NodeVector& circularReferences);
    void addFunctionForProcessing(Function* f); 
    void printCircularReferences();
    void processFunctionQueue(const Module& module, ExcludeFile& exclusions);
    ProcessingState processFunction(const Function& func, const Module& module, ExcludeFile& exclusions);

public:
    static char ID;

    TaintFlowPass() : ModulePass(ID), logger(errs(), nulls()) { }

    template<class TDep>
    TDep& getDependency(const Function& f) {
      return getAnalysis<TDep>(const_cast<Function&>(f));
    }

  };


#endif // TAINTFLOWPASS_H
