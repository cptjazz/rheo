#include "BlockHelper.h" 

bool BlockHelper::isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet) {
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
