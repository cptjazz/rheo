#ifndef INSTRUCTION_HANDLER_CONTEXT_H
#define INSTRUCTION_HANDLER_CONTEXT_H

#include <deque>


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
    InstructionHandlerContext(GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, raw_ostream& stream,
                              deque<const BasicBlock*>& worklist)
        : DT(dt), PDT(pdt), DOT(dot), stream(stream), enqueueBlockToWorklist(*new EnqueueToWorklistFunctor(worklist))
    { }

    const DominatorTree& DT;
    PostDominatorTree& PDT;
    GraphExporter& DOT;
    raw_ostream& stream;
    EnqueueToWorklistFunctor& enqueueBlockToWorklist;
};


#endif // INSTRUCTION_HANDLER_CONTEXT_H
