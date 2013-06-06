#ifndef STORE_HANDLER_H
#define STORE_HANDLER_H

#include "InstructionHandler.h"


class StoreHandler : public InstructionHandlerTrait<StoreInst> {

  public:
    StoreHandler(InstructionHandlerContext& ctx)
      : InstructionHandlerTrait(Instruction::Store, ctx) { }

    void handleInstructionInternal(const StoreInst& inst, TaintSet& taintSet) const;
};

#endif // STORE_HANDLER_H
