#ifndef BLOCK_HELPER_H
#define BLOCK_HELPER_H

#include "Core.h"
#include "llvm/Function.h"
#include "GraphExporter.h"
#include "llvm/Analysis/Dominators.h"

/**
 * The BlockHelper constructs a mapping of
 * successor to (set of) predecessor blocks
 * for a given function. This increases preformance
 * for predecessor relation lookups.
 */
class BlockHelper {
  const DominatorTree& DT;
  GraphExporter& DOT;
  raw_ostream& _stream;

  public:

    BlockHelper(const DominatorTree& dt, GraphExporter& dot, raw_ostream& stream) : DT(dt), DOT(dot), _stream(stream) { }

    bool isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet); 
};

#endif //BLOCK_HELPER_H
