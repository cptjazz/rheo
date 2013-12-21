#ifndef INSERT_VALUE_HANDLER_H
#define INSERT_VALUE_HANDLER_H

#include "InstructionHandler.h"


class InsertValueHandler : public InstructionHandlerTrait<InsertValueInst> {

  public:
    InsertValueHandler(InstructionHandlerContext& ctx)
      : InstructionHandlerTrait(Instruction::InsertValue, ctx) { }

    void handleInstructionInternal(const InsertValueInst& inst, TaintSet& taintSet) const;
};

#endif // INSERT_VALUE_HANDLER_H
