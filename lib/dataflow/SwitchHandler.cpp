#include "SwitchHandler.h"

void SwitchHandler::handleInstructionInternal(const SwitchInst& inst, TaintSet& taintSet) const {
  const Value* condition = inst.getCondition();

  if (!taintSet.contains(*condition))
    return;

  CTX.DOT.addRelation(*condition, inst, "switch");

  const size_t succCount = inst.getNumSuccessors();

  if (succCount == 1) {
    CTX.logger.debug() << "Skipping switch because it consisted solely of a default branch.\n";
    return;
  }

  const BasicBlock& defaultBlock = *inst.getSuccessor(0);

  bool canSwitchBeShortened = true;
  for (size_t i = 1; i < succCount; ++i) {
    const BasicBlock& caseBlock = *inst.getSuccessor(i);

    // If block consists of more than one instruction
    // it cannot be shortened.
    if (caseBlock.size() != 1) {
      canSwitchBeShortened = false;
      break;
    }

    // If the terminator has more than one successor
    // the switch cannot be shortened
    const TerminatorInst& terminator = *caseBlock.getTerminator();
    if (terminator.getNumSuccessors() != 1) {
      canSwitchBeShortened = false;
      break;
    }

    // Target must be default block to shorten
    if (terminator.getSuccessor(0) != &defaultBlock) {
      canSwitchBeShortened = false;
      break;
    }

    CTX.logger.debug() << "block size of " << inst.getSuccessor(i)->getName() << ": " << inst.getSuccessor(i)->size() << "\n";
  }

  if (canSwitchBeShortened) {
    CTX.logger.debug() << "Skipping switch because all cases fall through to default without executing other instructions before.\n";
    return;
  }

  const BasicBlock& join = *CTX.PDT.getNode(const_cast<BasicBlock*>(inst.getParent()))->getIDom()->getBlock();

  CTX.logger.debug() << " Handle SWITCH instruction:\n";
  CTX.logger.debug() << " Found joining block: " << join << "\n";

  for (size_t i = 0; i < succCount; ++i) {
    // Mark all case-blocks as tainted.
    const BasicBlock& caseBlock = *inst.getSuccessor(i);
    CTX.DOT.addBlockNode(caseBlock);
    CTX.DOT.addRelation(inst, caseBlock, "case");
    taintSet.add(caseBlock);
    CTX.logger.debug() << " + Added Block due to tainted SWITCH condition: " << caseBlock << "\n";

    CTX.BH.followTransientBranchPaths(caseBlock, join, taintSet);
  }
}
