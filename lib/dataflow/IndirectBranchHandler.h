#ifndef INDIRECTBRANCH_HANDLER_H
#define INDIRECTBRANCH_HANDLER_H

#include "InstructionHandler.h"


class IndirectBranchHandler : public UnsupportedInstructionHandlerTrait {

  public:
    IndirectBranchHandler(InstructionHandlerContext& ctx)
      : UnsupportedInstructionHandlerTrait(Instruction::IndirectBr, "Indirect branching is not supported", ctx) { }
};

#endif // INDIRECTBRANCH_HANDLER_H
