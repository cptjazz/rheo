#ifndef BLOCK_HELPER_H
#define BLOCK_HELPER_H

#include "Core.h"
#include "llvm/Function.h"

/**
 * The BlockHelper constructs a mapping of
 * successor to (set of) predecessor blocks
 * for a given function. This increases preformance
 * for predecessor relation lookups.
 */
class BlockHelper {
  typedef SmallPtrSet<const BasicBlock*, 8> BlockSet;

  // map<succ, preds>
  map<const BasicBlock*, BlockSet> successors;

  public:

    BlockHelper(const Function& f) {
      for (Function::const_iterator b_i = f.begin(), b_e = f.end(); b_i != b_e; ++b_i) {
        const BasicBlock* block = cast<BasicBlock>(b_i);

        BlockSet usedList;

        isCfgSuccessorInternal(block, &f.getEntryBlock(), usedList);
        successors.insert(make_pair(block, usedList));
      }
    }

    inline bool isSuccessor(const BasicBlock* succ, const BasicBlock* pred) {
      return successors[succ].count(pred);
    }

  private:
    inline bool isCfgSuccessorInternal(const BasicBlock* succ, const BasicBlock* pred, BlockSet& usedList) {
      if (NULL == succ || NULL == pred)
        return false;

      if (pred == succ)
        return true;

      for (const_pred_iterator i = pred_begin(succ), e = pred_end(succ); i != e; ++i) {
        if (usedList.count(*i))
          continue;

        usedList.insert(*i);

        if (isCfgSuccessorInternal(*i, pred, usedList))
          return true;
      }

      return false;
    }
};

#endif //BLOCK_HELPER_H
