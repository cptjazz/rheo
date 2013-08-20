#include "BlockHelper.h" 

bool BlockHelper::isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet) const {
  for (TaintSet::const_iterator s_i = taintSet.begin(), s_e = taintSet.end(); s_i != s_e; ++s_i) {
    const Value* v = *s_i;

    const BasicBlock* taintedBlock = dyn_cast_or_null<BasicBlock>(v);
    if (!taintedBlock)
      continue;

    if (DT.dominates(taintedBlock, &currentBlock)) {
      DEBUG(logger.debug() << " ! Dirty block `" << taintedBlock->getName() << "` dominates `" << currentBlock.getName() << "`\n");

      return true;
    }
  }

  return false;
}

void BlockHelper::followTransientBranchPaths(const BasicBlock& br, const BasicBlock& join, TaintSet& taintSet) const {
  const TerminatorInst& brTerminator = *br.getTerminator();

  if (brTerminator.getNumSuccessors() != 1)
    return;

  const BasicBlock* brSuccessor = brTerminator.getSuccessor(0);

  if (PDT.dominates(&join, brSuccessor) && brSuccessor != &join) {

    IF_GRAPH(DOT.addBlockNode(*brSuccessor));
    IF_GRAPH(DOT.addRelation(brTerminator, *brSuccessor, "block-taint"));
    DEBUG(logger.debug() << " ++ Added TRANSIENT block: " << brSuccessor->getName() << "\n");

    taintSet.add(*brSuccessor);
    followTransientBranchPaths(*brSuccessor, join, taintSet);
  }
}
