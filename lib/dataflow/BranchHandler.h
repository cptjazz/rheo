#ifndef BRANCH_HANDLER_H
#define BRANCH_HANDLER_H

#include "InstructionHandler.h"

class BranchHandler : public InstructionHandlerTrait<BranchInst> {
  private:
    void handleConditionalBranch(const BranchInst& inst, TaintSet& taintSet) const;
    void handleUnconditionalBranch(const BranchInst& inst, TaintSet& taintSet) const;
  public:
    BranchHandler(InstructionHandlerContext& ctx)
      : InstructionHandlerTrait(Instruction::Br, ctx) { }

    void handleInstructionInternal(const BranchInst& inst, TaintSet& taintSet) const;
};

#endif // BRANCH_HANDLER_H
