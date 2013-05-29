#ifndef INDIRECTBRANCH_HANDLER_H
#define INDIRECTBRANCH_HANDLER_H

#include "InstructionHandler.h"


class IndirectBranchHandler : public UnsupportedInstructionHandlerTrait<IndirectBrInst> {

  public:
    IndirectBranchHandler(InstructionHandlerContext& ctx)
      : UnsupportedInstructionHandlerTrait("Indirect branching is not supported", ctx) { }
};

#endif // INDIRECTBRANCH_HANDLER_H
