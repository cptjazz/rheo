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

  const BasicBlock& brTrue = *inst.getSuccessor(0);
  const BasicBlock& brFalse = *inst.getSuccessor(1);

  const BasicBlock& join = *const_cast<const BasicBlock*>(CTX.PDT.findNearestCommonDominator(const_cast<BasicBlock*>(&brFalse), const_cast<BasicBlock*>(&brTrue)));

  DEBUG(CTX.logger.debug() << "   Nearest Common Post-Dominator for tr/fa: " << join << "\n");

  // true branch is always tainted
  taintSet.add(brTrue);
  DEBUG(CTX.DOT.addBlockNode(brTrue));
  DEBUG(CTX.DOT.addRelation(inst, brTrue, "br-true"));
  DEBUG(CTX.logger.debug() << " + Added TRUE branch to taint set:\n");
  DEBUG(CTX.logger.debug() << brTrue.getName() << "\n");

  CTX.BH.followTransientBranchPaths(brTrue, join, taintSet);

  // false branch is only tainted if successor
  // is not the same as jump target after true branch
  if (&join != &brFalse) {
    taintSet.add(brFalse);
    DEBUG(CTX.DOT.addBlockNode(brFalse));
    DEBUG(CTX.DOT.addRelation(inst, brFalse, "br-false"));
    DEBUG(CTX.logger.debug() << " + Added FALSE branch to taint set:\n");
    DEBUG(CTX.logger.debug() << brFalse.getName() << "\n");

    CTX.BH.followTransientBranchPaths(brFalse, join, taintSet);
  }
}

void BranchHandler::handleUnconditionalBranch(const BranchInst& inst, TaintSet& taintSet) const {
  if (taintSet.hasChanged()) {
    const BasicBlock* target = inst.getSuccessor(0);
    CTX.enqueueBlockToWorklist(target);
    DEBUG(CTX.logger.debug() << "Added block " << target->getName() << " for reinspection due to UNCONDITIONAL JUMP\n");
  }
}
