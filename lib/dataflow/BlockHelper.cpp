#include "BlockHelper.h" 

bool BlockHelper::isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet) const {
  bool result = false;

  for (TaintSet::const_iterator s_i = taintSet.begin(), s_e = taintSet.end(); s_i != s_e; ++s_i) {
    if (*s_i == NULL)
      continue;

    assert (*s_i != NULL && "BB must not be NULL");
    if (! isa<BasicBlock>(*s_i))
      continue;

    const BasicBlock& taintedBlock = cast<BasicBlock>(**s_i);

    if (DT.dominates(&taintedBlock, &currentBlock)) {
      DEBUG_LOG(" ! Dirty block `" << taintedBlock.getName() << "` dominates `" << currentBlock.getName() << "`\n");

      if (&taintedBlock != &currentBlock) {
        taintSet.add(currentBlock);
        DOT.addBlockNode(currentBlock);
        DOT.addRelation(taintedBlock, currentBlock, "block-taint");
      }

      result = true;
    }
  }

  return result;
}

void BlockHelper::followTransientBranchPaths(const BasicBlock& br, const BasicBlock& join, TaintSet& taintSet) const {
  const TerminatorInst& brTerminator = *br.getTerminator();

  if (brTerminator.getNumSuccessors() != 1)
    return;

  const BasicBlock& brSuccessor = *brTerminator.getSuccessor(0);

  if (PDT.dominates(&join, &brSuccessor) &&
      &brSuccessor != &join) {

    taintSet.add(brSuccessor);
    DOT.addBlockNode(brSuccessor);
    DOT.addRelation(brTerminator, brSuccessor, "block-taint");
    DEBUG_LOG(" ++ Added TRANSIENT block:\n");
    DEBUG_LOG(brSuccessor << "\n");

    followTransientBranchPaths(brSuccessor, join, taintSet);
  }
}
