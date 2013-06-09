#ifndef BLOCK_HELPER_H
#define BLOCK_HELPER_H

#include "Core.h"
#include "GraphExporter.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "Logger.h"


class BlockHelper {
  const DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter& DOT;
  const Logger& logger;

  public:

    BlockHelper(const DominatorTree& dt, PostDominatorTree& pdt, GraphExporter& dot, const Logger& logger)
        : DT(dt), PDT(pdt), DOT(dot), logger(logger) { }

    bool isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet) const;

    void followTransientBranchPaths(const BasicBlock& br, const BasicBlock& join, TaintSet& taintSet) const;
};

#endif //BLOCK_HELPER_H
