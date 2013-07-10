#ifndef INT2PTR_HANDLER_H
#define INT2PTR_HANDLER_H

#include "InstructionHandler.h"


class IntToPtrHandler : public UnsupportedInstructionHandlerTrait {

  public:
    IntToPtrHandler(InstructionHandlerContext& ctx)
      : UnsupportedInstructionHandlerTrait(Instruction::IntToPtr, "IntToPtr is not supported", ctx) { }
};

#endif // INT2PTR_HANDLER_H
