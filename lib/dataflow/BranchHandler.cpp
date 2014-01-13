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

  IF_GRAPH(CTX.DOT.addRelation(cmp, inst, "condition"));

  const BasicBlock& br1 = *inst.getSuccessor(0);
  const BasicBlock& br2 = *inst.getSuccessor(1);

  const BasicBlock& join1 = *CTX.PDT.findNearestCommonDominator(&br2, &br1);
  const BasicBlock& join2 = *CTX.PDT.findNearestCommonDominator(&br1, &br2);

  // We need to check this twice. There are special constructs (a block terminated by BR and no 
  // join-block for the branches. This can happen if both branches loop infinitely and all
  // successing blocks were "optimised away", as can happen with "-O3".
  // findNearestCommonDominator then always yields the 2nd argument as dominator.
  DEBUG(CTX.logger.debug() << "   Nearest Common Post-Dominator for fa/tr: " << join1.getName() << "\n");
  DEBUG(CTX.logger.debug() << "   Nearest Common Post-Dominator for tr/fa: " << join2.getName() << "\n");

  // branch is only tainted if successor
  // is not the same as jump target after true branch
  if (&join1 != &br1) {
    taintSet.add(br1);
    IF_GRAPH(CTX.DOT.addBlockNode(br1));
    IF_GRAPH(CTX.DOT.addRelation(inst, br1, "br"));
    DEBUG(CTX.logger.debug() << " + Added 1st branch to taint set: " << br1.getName() << "\n");

    CTX.BH.followTransientBranchPaths(br1, join1, taintSet);
  }

  // branch is only tainted if successor
  // is not the same as jump target after true branch
  if (&join2 != &br2) {
    taintSet.add(br2);
    IF_GRAPH(CTX.DOT.addBlockNode(br2));
    IF_GRAPH(CTX.DOT.addRelation(inst, br2, "br"));
    DEBUG(CTX.logger.debug() << " + Added 2nd branch to taint set: " << br2.getName() << "\n");

    CTX.BH.followTransientBranchPaths(br2, join2, taintSet);
  }
}

void BranchHandler::handleUnconditionalBranch(const BranchInst& inst, TaintSet& taintSet) const {
  if (!taintSet.hasChanged())
    return;
 
  const BasicBlock* target = inst.getSuccessor(0);
  CTX.enqueueBlockToWorklist(target);
  DEBUG(CTX.logger.debug() << "Added block " << target->getName() << " for reinspection due to UNCONDITIONAL JUMP\n");
}
