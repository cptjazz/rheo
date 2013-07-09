#ifndef INSTRUCTION_HANDLER_CONTEXT_H
#define INSTRUCTION_HANDLER_CONTEXT_H

#include <deque>
#include "Core.h"
#include "AnalysisState.h"
#include "SetHelper.h"
#include "TaintFlowPass.h"
#include "GraphExporter.h"
#include "BlockHelper.h"
#include "ExcludeFile.h"

struct EnqueueToWorklistFunctor {
    deque<const BasicBlock*>& _worklist;

    EnqueueToWorklistFunctor(deque<const BasicBlock*>& worklist)
      : _worklist(worklist) { }

    void operator()(const BasicBlock* block) {
      if (_worklist.front() != block)
        _worklist.push_front(block);
    }
};


class InstructionHandlerContext {
  public:
    InstructionHandlerContext(GraphExporter& dot, DominatorTree& dt, PostDominatorTree& pdt, const Logger& logger,
                              deque<const BasicBlock*>& worklist, AnalysisState& analysisState, const Function& f,
                              CircleMap& circRef, SetHelper& setHelper, TaintFlowPass& pass, const Module& module,
                              ExcludeFile& exclusions)
        : DT(dt), PDT(pdt), DOT(dot), logger(logger), enqueueBlockToWorklist(worklist), analysisState(analysisState),
          BH(dt, pdt, dot, logger), F(f), circularReferences(circRef), setHelper(setHelper), PASS(pass), M(module),
          EXCL(exclusions)
    { }

    DominatorTree& DT;
    PostDominatorTree& PDT;
    GraphExporter& DOT;
    const Logger& logger;
    EnqueueToWorklistFunctor enqueueBlockToWorklist;
    AnalysisState& analysisState;
    BlockHelper BH;
    const Function& F;
    CircleMap& circularReferences;
    SetHelper& setHelper;
    TaintFlowPass& PASS;
    const Module& M;
    map<const CallInst*, ResultSet> mappingCache;
    ExcludeFile& EXCL;

    void refreshDomTrees() {
      DT.runOnFunction(const_cast<Function&>(F));
      PDT.runOnFunction(const_cast<Function&>(F));
    }
};


#endif // INSTRUCTION_HANDLER_CONTEXT_H
