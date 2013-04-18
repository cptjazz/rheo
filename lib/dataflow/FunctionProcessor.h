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
#include "NullGraphExporter.h"
#include "Helper.h"
#include "Core.h"
#include "TaintFlowPass.h" 


using namespace llvm;
using namespace std;


class FunctionProcessor {
  const Function& F;
  const DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter* DOT;
  const Module& M;
  TaintFlowPass& PASS;

  TaintMap _returnStatements;
  TaintMap _arguments;
  ResultSet& _taints;
  map<const BasicBlock*, TaintSet> _blockList;
  deque<const BasicBlock*> _workList;
  FunctionMap& _circularReferences;

  raw_ostream& _stream;

  bool _canceledInspection;
  bool _taintSetChanged;
  bool _resultSetChanged;
  bool _suppressPrintTaints;

public:
  FunctionProcessor(TaintFlowPass& pass, const Function& f, FunctionMap& circRef, const Module& m, ResultSet& result, raw_ostream& stream) 
  : F(f), DT(pass.getDependency<DominatorTree>(f)), PDT(pass.getDependency<PostDominatorTree>(f)), M(m),
    PASS(pass), _taints(result), _circularReferences(circRef), _stream(stream)
  { 
    _canceledInspection = false;
    _suppressPrintTaints = false;

    DOT = new NullGraphExporter();

    DEBUG(delete DOT);
    DEBUG(DOT = new GraphExporter(f.getName()));
  }

  ~FunctionProcessor() {
    delete DOT;
  }

  void processFunction();
  bool didFinish();

private:
  void intersectSets(const Value& arg, const TaintSet argTaintSet, const bool debugPrintSet);
  void buildResultSet(const bool debugPrintSet);
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
  void handlePhiNode(const PHINode& inst, TaintSet& taintSet);

  void findArguments();
  void handleFoundArgument(const Value& arg);
  void findAllStoresAndLoadsForOutArgumentAndAddToSet(const Value& arg, ReturnSet& retlist);
  void printSet(const TaintSet& s);
  void findReturnStatements();
  void printInstructions(); 
  void readTaintsFromFile(const CallInst& callInst, const Function& func, ResultSet& taintResults);
  bool isCfgSuccessor(const BasicBlock* succ, const BasicBlock* pred, set<const BasicBlock*>& usedList);
  bool isCfgSuccessorOfPreviousStores(const StoreInst& storeInst, const TaintSet& taintSet);
  void recursivelyAddAllGeps(const GetElementPtrInst& gep, TaintSet& taintSet);
  void recursivelyFindAliases(const Value& arg, ReturnSet& taintSet, ReturnSet& alreadyProcessed);
  void followTransientBranchPaths(const BasicBlock& br, const BasicBlock& join, TaintSet& taintSet);
  void addTaintToSet(TaintSet& taintSet, const Value& v);
  bool isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet);
  void applyMeet(const BasicBlock& block);
  void enqueueBlockToWorklist(const BasicBlock* block);
  int getArgumentPosition(const CallInst& c, const Value& v);
  int getArgumentPosition(const Function& f, const Value& v);
  void buildMappingForRecursiveCall(const CallInst& callInst, const Function& func, ResultSet& taintResults);
  void buildMappingForCircularReferenceCall(const CallInst& callInst, const Function& func, ResultSet& taintResults);
  void buildMappingForUndefinedExternalCall(const CallInst& callInst, const Function& func, ResultSet& taintResults);
  void createResultSetFromFunctionMapping(const CallInst& callInst, FunctionTaintMap& mapping, ResultSet& taintResults);

  inline raw_ostream& debug() {
    return _stream;
  }

  inline raw_ostream& release() {
    return _stream;
  }
};

#endif // FUNCTION_PROCESSOR_H
