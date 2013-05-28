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
#include "BlockHelper.h" 
#include "InstructionHandler.h" 
#include "InstructionHandlerDispatcher.h" 
#include "GetElementPtrHandler.h" 
#include "PhiNodeHandler.h" 
#include "DefaultHandler.h" 


using namespace llvm;
using namespace std;


class FunctionProcessor {
  const Function& F;
  const DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter* DOT;
  const Module& M;
  TaintFlowPass& PASS;
  InstructionHandlerDispatcher* IHD;
  BlockHelper* BH;

  TaintMap _returnStatements;
  TaintMap _arguments;
  ResultSet& _taints;
  map<const BasicBlock*, TaintSet> _blockList;
  deque<const BasicBlock*> _workList;
  CircleMap& _circularReferences;

  raw_ostream& _stream;

  bool _canceledInspection;
  bool _resultSetChanged;
  bool _suppressPrintTaints;
  bool _shouldWriteErrors;
  Function* _missingDefinition;
  ProcessingState _processingState;

public:
  FunctionProcessor(TaintFlowPass& pass, const Function& f, CircleMap& circRef, const Module& m, ResultSet& result, raw_ostream& stream) 
  : F(f), DT(pass.getDependency<DominatorTree>(f)), PDT(pass.getDependency<PostDominatorTree>(f)), M(m),
    PASS(pass), _taints(result), _circularReferences(circRef), _stream(stream)
  { 
    _canceledInspection = false;
    _suppressPrintTaints = false;
    _shouldWriteErrors = true;

    DOT = new NullGraphExporter();

    DEBUG(delete DOT);
    DEBUG(DOT = new GraphExporter(f.getName()));

    BH = new BlockHelper(DT, *DOT, stream);
    registerHandlers();
  }

  ~FunctionProcessor() {
    delete DOT;
    delete BH;
    delete IHD;
  }

  void processFunction();

  void setMissingDefinition(const Function* f) {
    _missingDefinition = const_cast<Function*>(f);
  }

  const Function* getMissingDefinition() {
    return _missingDefinition;
  }

  ProcessingState getState() {
    return _processingState;
  }

  bool didFinish() {
    return !_canceledInspection;
  }

  void setShouldWriteErrors(bool val) {
    _shouldWriteErrors = val;
  }

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
  void handleFunctionCall(const CallInst& callInst, const Function& callee, TaintSet& taintSet);
  void handleBranchInstruction(const BranchInst& inst, TaintSet& taintSet);
  void handleSwitchInstruction(const SwitchInst& inst, TaintSet& taintSet);
  void handleInstruction(const Instruction& inst, TaintSet& taintSet);
  void handleBlockTainting(const Instruction& inst, const BasicBlock& currentblock, TaintSet& taintSet);
  void handlePhiNode(const PHINode& inst, TaintSet& taintSet);

  void findArguments();
  void handleFoundArgument(const Value& arg);
  void findAllStoresAndLoadsForOutArgumentAndAddToSet(const Value& arg, ReturnSet& retlist);
  void findReturnStatements();
  void printInstructions(); 
  void buildMappingFromTaintFile(const CallInst& callInst, const Function& func, ResultSet& taintResults);
  bool isCfgSuccessor(const BasicBlock* succ, const BasicBlock* pred, set<const BasicBlock*>& usedList);
  bool isCfgSuccessorOfPreviousStores(const StoreInst& storeInst, const TaintSet& taintSet);
  void recursivelyAddAllGepsAndLoads(const Instruction& target, TaintSet& taintSet);
  void recursivelyFindAliases(const Value& arg, ReturnSet& taintSet, ReturnSet& alreadyProcessed);
  void followTransientBranchPaths(const BasicBlock& br, const BasicBlock& join, TaintSet& taintSet);
  bool isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet);
  void applyMeet(const BasicBlock& block);
  void enqueueBlockToWorklist(const BasicBlock* block);
  int getArgumentPosition(const CallInst& c, const Value& v);
  int getArgumentPosition(const Function& f, const Value& v);
  void buildMappingForRecursiveCall(const CallInst& callInst, const Function& func, ResultSet& taintResults);
  void buildMappingForCircularReferenceCall(const CallInst& callInst, const Function& func, ResultSet& taintResults);
  void buildMappingForUndefinedExternalCall(const CallInst& callInst, const Function& func, ResultSet& taintResults);
  void createResultSetFromFunctionMapping(const CallInst& callInst, const Function& callee, const FunctionTaintMap& mapping, ResultSet& taintResults);
  void processFunctionCallResultSet(const CallInst& callInst, const Value& callee, ResultSet& taintResults, TaintSet& taintSet);
  void handleFunctionPointerCallWithHeuristic(const CallInst& callInst, TaintSet& taintSet);
  void stopWithError(Twine msg, ProcessingState state);

  void registerHandlers() {
    IHD = new InstructionHandlerDispatcher(*DOT, DT, PDT, _stream);

    IHD->registerDefaultHandler<DefaultHandler>();
    IHD->registerHandler<GetElementPtrHandler>();
    IHD->registerHandler<PhiNodeHandler>();
  }

};

#endif // FUNCTION_PROCESSOR_H
