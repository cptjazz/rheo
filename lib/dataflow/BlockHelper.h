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

  public:
    inline static bool isSuccessor(const BasicBlock* succ, const BasicBlock* pred) {
      BlockSet usedList;
      return isCfgSuccessorInternal(succ, pred, usedList);
    }

  private:
    inline static bool isCfgSuccessorInternal(const BasicBlock* succ, const BasicBlock* pred, BlockSet& usedList) {
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
