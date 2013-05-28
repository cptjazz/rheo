#ifndef BLOCK_HELPER_H
#define BLOCK_HELPER_H

#include "Core.h"
#include "llvm/Function.h"
#include "GraphExporter.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"


class BlockHelper {
  const DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter& DOT;
  raw_ostream& _stream;

  public:

    BlockHelper(const DominatorTree& dt, PostDominatorTree& pdt, GraphExporter& dot, raw_ostream& stream)
        : DT(dt), PDT(pdt), DOT(dot), _stream(stream) { }

    bool isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet) const;

    void followTransientBranchPaths(const BasicBlock& br, const BasicBlock& join, TaintSet& taintSet) const;
};

#endif //BLOCK_HELPER_H
