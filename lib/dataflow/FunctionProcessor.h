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
#include <cstring>
#include <stdio.h>
#include "GraphExporter.h"
#include "Helper.h"
#include "Core.h"


using namespace llvm;
using namespace std;


class FunctionProcessor {
  Function& F;
  DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter DOT;
  Module& M;

  TaintMap _returnStatements;
  TaintMap _arguments;
  ResultSet& _taints;

  raw_ostream& _stream;

  bool canceledInspection;

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
  void intersectSets(Value& arg, TaintSet argTaintSet);
  void buildTaintSetFor(Value& arg, TaintSet& taintSet);
  void addTaint(Value& tainter, Value& taintee);
  void processBasicBlock(BasicBlock& block, TaintSet& taintSet);
  void printTaints();
  void handleGetElementPtrInstruction(GetElementPtrInst& storeInst, TaintSet& taintSet);
  void handleStoreInstruction(StoreInst& storeInst, TaintSet& taintSet);
  void handleCallInstruction(CallInst& callInst, TaintSet& taintSet);
  void handleBranchInstruction(BranchInst& inst, TaintSet& taintSet);
  void handleSwitchInstruction(SwitchInst& inst, TaintSet& taintSet);
  void handleInstruction(Instruction& inst, TaintSet& taintSet);
  bool handleBlockTainting(TaintSet& taintSet, Instruction& inst);
  void findArguments();
  void handleFoundArgument(Value& arg);
  void findAllStoresAndLoadsForOutArgumentAndAddToSet(Value& arg, TaintSet& retlist);
  void printSet(TaintSet& s);
  void findReturnStatements();
  void printInstructions(); 
  void readTaintsFromFile(TaintSet& taintSet, CallInst& callInst, Function& func, ResultSet& result);
  bool isCfgSuccessor(BasicBlock* succ, BasicBlock* pred, set<BasicBlock*>& usedList);
  bool isCfgSuccessorOfPreviousStores(StoreInst& storeInst, TaintSet& taintSet);
  void recursivelyAddAllGeps(GetElementPtrInst& gep, TaintSet& taintSet);
  void recursivelyFindAliases(Value& arg, TaintSet& taintSet, TaintSet& alreadyProcessed);
  void followTransientBranchPaths(BasicBlock& br, BasicBlock& join, TaintSet& taintSet);

  raw_ostream& debug() {
    return _stream;
  }

  raw_ostream& release() {
    return _stream;
  }
};

#endif // FUNCTION_PROCESSOR_H
