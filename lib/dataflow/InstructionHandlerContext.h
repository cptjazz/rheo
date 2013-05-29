#ifndef INSTRUCTION_HANDLER_CONTEXT_H
#define INSTRUCTION_HANDLER_CONTEXT_H

#include <deque>
#include "AnalysisState.h"
#include "llvm/Function.h"
#include "SetHelper.h"
#include "TaintFlowPass.h"

struct EnqueueToWorklistFunctor {
    deque<const BasicBlock*>& _worklist;

    EnqueueToWorklistFunctor(deque<const BasicBlock*>& worklist)
      : _worklist(worklist) { }

    void operator()(const BasicBlock* block) {
      _worklist.push_front(block);
    }
};


class InstructionHandlerContext {
  public:
    InstructionHandlerContext(GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, const Logger& logger,
                              deque<const BasicBlock*>& worklist, AnalysisState& analysisState, const Function& f,
                              CircleMap& circRef, SetHelper& setHelper, TaintFlowPass& pass, const Module& module)
        : DT(dt), PDT(pdt), DOT(dot), logger(logger), enqueueBlockToWorklist(worklist), analysisState(analysisState),
          BH(dt, pdt, dot, logger), F(f), circularReferences(circRef), setHelper(setHelper), PASS(pass), M(module)
    { }

    const DominatorTree& DT;
    PostDominatorTree& PDT;
    GraphExporter& DOT;
    const Logger& logger;
    EnqueueToWorklistFunctor enqueueBlockToWorklist;
    AnalysisState analysisState;
    BlockHelper BH;
    const Function& F;
    CircleMap& circularReferences;
    SetHelper& setHelper;
    TaintFlowPass& PASS;
    const Module& M;
};


#endif // INSTRUCTION_HANDLER_CONTEXT_H
