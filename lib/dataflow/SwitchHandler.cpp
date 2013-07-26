#include "SwitchHandler.h"

void SwitchHandler::handleInstructionInternal(const SwitchInst& inst, TaintSet& taintSet) const {
  const Value* condition = inst.getCondition();

  if (!taintSet.contains(*condition))
    return;

  DEBUG(CTX.DOT.addRelation(*condition, inst, "switch"));

  if (!inst.getNumCases()) {
    DEBUG(CTX.logger.debug() << "Skipping switch because it consisted solely of a default branch.\n");
    return;
  }

  const BasicBlock* defaultBlock = inst.getDefaultDest();
  const BasicBlock& join = *CTX.PDT.getNode(const_cast<BasicBlock*>(inst.getParent()))->getIDom()->getBlock();

  DEBUG(CTX.logger.debug() << " Handle SWITCH instruction:\n");
  DEBUG(CTX.logger.debug() << " Found joining block: " << join << "\n");

  // If no default block exists, the default jump target 
  // (Successor0) is after the switch and equals the join-block.
  // Otherwise, an explicit 'default' label was provided and it has
  // to be treated like the other blocks.
  unsigned int startBlock = (&join == defaultBlock) ? 1 : 0;
  
  const size_t succCount = inst.getNumSuccessors();

  for (size_t i = startBlock; i < succCount; ++i) {
    // Mark all case-blocks as tainted.
    const BasicBlock& caseBlock = *inst.getSuccessor(i);
    DEBUG(CTX.DOT.addBlockNode(caseBlock));
    DEBUG(CTX.DOT.addRelation(inst, caseBlock, "case"));
    DEBUG(CTX.logger.debug() << " + Added Block due to tainted SWITCH condition: " << caseBlock << "\n");

    taintSet.add(Taint::make_infered(caseBlock));
    CTX.BH.followTransientBranchPaths(caseBlock, join, taintSet);
  }
}
