#ifndef INDIRECTBRANCH_HANDLER_H
#define INDIRECTBRANCH_HANDLER_H

#include "InstructionHandler.h"


class IndirectBranchHandler : public UnsupportedInstructionHandlerTrait {

  public:
    IndirectBranchHandler(InstructionHandlerContext& ctx)
      : UnsupportedInstructionHandlerTrait(Instruction::IndirectBr, "Indirect branching is not supported", ctx) { }
};

class IntToPtrHandler : public UnsupportedInstructionHandlerTrait {

  public:
    IntToPtrHandler(InstructionHandlerContext& ctx)
      : UnsupportedInstructionHandlerTrait(Instruction::IntToPtr, "IntToPtr is not supported", ctx) { }
};

class InvokeHandler : public UnsupportedInstructionHandlerTrait {

  public:
    InvokeHandler(InstructionHandlerContext& ctx)
      : UnsupportedInstructionHandlerTrait(Instruction::Invoke, "Invoke is not supported. No exception handling.", ctx) { }
};

class ResumeHandler : public UnsupportedInstructionHandlerTrait {

  public:
    ResumeHandler(InstructionHandlerContext& ctx)
      : UnsupportedInstructionHandlerTrait(Instruction::IntToPtr, "Resume is not supported. No exception handling.", ctx) { }
};

class LandingPadHandler : public UnsupportedInstructionHandlerTrait {

  public:
    LandingPadHandler(InstructionHandlerContext& ctx)
      : UnsupportedInstructionHandlerTrait(Instruction::LandingPad, "LandingPad is not supported. No exception handling.", ctx) { }
};

#endif // INDIRECTBRANCH_HANDLER_H
