#ifndef FUNCTION_PROCESSOR_H
#define FUNCTION_PROCESSOR_H

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/InstrTypes.h"
#include <algorithm>
#include <deque>
#include <cstring>
#include <stdio.h>
#include "GraphExporter.h"
#include "Helper.h"
#include "Core.h"


using namespace llvm;
using namespace std;


class FunctionProcessor {
  const Function& F;
  const DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter DOT;
  const Module& M;

  TaintMap _returnStatements;
  TaintMap _arguments;
  ResultSet& _taints;
  map<const BasicBlock*, TaintSet> _blockList;
  deque<const BasicBlock*> _workList;

  raw_ostream& _stream;

  bool canceledInspection;
  bool _taintSetChanged;
  bool _resultSetChanged;

public:
  FunctionProcessor(Function& f, Module& m, DominatorTree& dt, PostDominatorTree& pdt, ResultSet& result, raw_ostream& stream) 
  : F(f), DT(dt), PDT(pdt), DOT(f.getName()), M(m), _taints(result), _stream(stream)  { 
    canceledInspection = false;
  }

  ~FunctionProcessor() {
  }

  void processFunction();
  bool didFinish();

private:
  void intersectSets(const Value& arg, const TaintSet argTaintSet);
  void buildResultSet();
  void buildTaintSetFor(const Value& arg, TaintSet& taintSet);
  void addTaint(const Value& tainter, const Value& taintee);
  void processBasicBlock(const BasicBlock& block, TaintSet& taintSet);
  void printTaints();

  void handleGetElementPtrInstruction(const GetElementPtrInst& storeInst, TaintSet& taintSet);
  void handleStoreInstruction(const StoreInst& storeInst, TaintSet& taintSet);
  void handleCallInstruction(const CallInst& callInst, TaintSet& taintSet);
  void handleBranchInstruction(const BranchInst& inst, TaintSet& taintSet);
  void handleSwitchInstruction(const SwitchInst& inst, TaintSet& taintSet);
  void handleInstruction(const Instruction& inst, TaintSet& taintSet);
  void handleBlockTainting(const Instruction& inst, const BasicBlock& currentblock, TaintSet& taintSet);

  void findArguments();
  void handleFoundArgument(const Value& arg);
  void findAllStoresAndLoadsForOutArgumentAndAddToSet(const Value& arg, ReturnSet& retlist);
  void printSet(const TaintSet& s);
  void findReturnStatements();
  void printInstructions(); 
  void readTaintsFromFile(TaintSet& taintSet, const CallInst& callInst, const Function& func);
  bool isCfgSuccessor(const BasicBlock* succ, const BasicBlock* pred, set<const BasicBlock*>& usedList);
  bool isCfgSuccessorOfPreviousStores(const StoreInst& storeInst, const TaintSet& taintSet);
  void recursivelyAddAllGeps(const GetElementPtrInst& gep, TaintSet& taintSet);
  void recursivelyFindAliases(const Value& arg, ReturnSet& taintSet, ReturnSet& alreadyProcessed);
  void followTransientBranchPaths(const BasicBlock& br, const BasicBlock& join, TaintSet& taintSet);
  void addTaintToSet(TaintSet& taintSet, const Value& v);
  bool isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet);
  void applyMeet(const BasicBlock& block);
  void enqueueBlockToWorklist(const BasicBlock* block);

  inline raw_ostream& debug() {
    return _stream;
  }

  inline raw_ostream& release() {
    return _stream;
  }
};

#endif // FUNCTION_PROCESSOR_H
