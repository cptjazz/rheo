#ifndef GETELEMENTPRT_HANDLER_H
#define GETELEMENTPRT_HANDLER_H

#include "InstructionHandler.h"

class GetElementPtrHandler : public InstructionHandlerTrait<GetElementPtrInst> {

  public:
    GetElementPtrHandler(InstructionHandlerContext& ctx)
      : InstructionHandlerTrait(Instruction::GetElementPtr, ctx) { }

    void handleInstructionInternal(const GetElementPtrInst& inst, TaintSet& taintSet) const;
};

#endif // GETELEMENTPRT_HANDLER_H
