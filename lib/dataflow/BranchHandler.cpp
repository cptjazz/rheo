#include "BranchHandler.h"

void BranchHandler::handleInstructionInternal(const BranchInst& inst, TaintSet& taintSet) const {
  DEBUG(CTX.logger.debug() << " Handle BRANCH instruction: " << inst << "\n");

  if (inst.isConditional())
    handleConditionalBranch(inst, taintSet);
  else
    handleUnconditionalBranch(inst, taintSet);
}

void BranchHandler::handleConditionalBranch(const BranchInst& inst, TaintSet& taintSet) const {
  const Value& cmp = *inst.getCondition();
  DEBUG(CTX.logger.debug() << " = Compare instruction is: " << cmp << "\n");

  bool isConditionTainted = taintSet.contains(cmp);
  if (!isConditionTainted) 
    return;

  DEBUG(CTX.DOT.addRelation(cmp, inst, "condition"));

  const BasicBlock& br1 = *inst.getSuccessor(0);
  const BasicBlock& br2 = *inst.getSuccessor(1);

  const BasicBlock& join = *const_cast<const BasicBlock*>(
      CTX.PDT.findNearestCommonDominator(const_cast<BasicBlock*>(&br1), const_cast<BasicBlock*>(&br2)));

  DEBUG(CTX.logger.debug() << "   Nearest Common Post-Dominator for tr/fa: " << join.getName() << "\n");

  // true branch is always tainted
  // branch is only tainted if successor
  // is not the same as jump target after true branch
  if (&join != &br1) {
    taintSet.add(br1);
    DEBUG(CTX.DOT.addBlockNode(br1));
    DEBUG(CTX.DOT.addRelation(inst, br1, "br"));
    DEBUG(CTX.logger.debug() << " + Added branch to taint set: " << br1.getName() << "\n");

    CTX.BH.followTransientBranchPaths(br1, join, taintSet);
  }

  // branch is only tainted if successor
  // is not the same as jump target after true branch
  if (&join != &br2) {
    taintSet.add(br2);
    DEBUG(CTX.DOT.addBlockNode(br2));
    DEBUG(CTX.DOT.addRelation(inst, br2, "br"));
    DEBUG(CTX.logger.debug() << " + Added FALSE branch to taint set: " << br2.getName() << "\n");

    CTX.BH.followTransientBranchPaths(br2, join, taintSet);
  }
}

void BranchHandler::handleUnconditionalBranch(const BranchInst& inst, TaintSet& taintSet) const {
  if (taintSet.hasChanged()) {
    const BasicBlock* target = inst.getSuccessor(0);
    CTX.enqueueBlockToWorklist(target);
    DEBUG(CTX.logger.debug() << "Added block " << target->getName() << " for reinspection due to UNCONDITIONAL JUMP\n");
  }
}
