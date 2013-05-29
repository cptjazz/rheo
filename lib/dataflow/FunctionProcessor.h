#ifndef FUNCTION_PROCESSOR_H
#define FUNCTION_PROCESSOR_H

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/InstrTypes.h"
#include <algorithm>
#include <deque>
#include <cstring>
#include <stdio.h>
#include "Logger.h"
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
#include "BranchHandler.h"
#include "SwitchHandler.h"
#include "StoreHandler.h"
#include "IndirectBranchHandler.h"
#include "CallHandler.h"
#include "DefaultHandler.h"
#include "Logger.h"
#include "SetHelper.h"

using namespace llvm;
using namespace std;


class FunctionProcessor {
  const Function& F;
  const DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter* DOT;
  const Module& M;
  const Logger& logger;
  TaintFlowPass& PASS;
  SetHelper setHelper;
  InstructionHandlerDispatcher* IHD;
  InstructionHandlerContext* CTX;
  BlockHelper* BH;

  map<const BasicBlock*, TaintSet> _blockList;
  deque<const BasicBlock*> _workList;
  CircleMap& _circularReferences;

  bool _suppressPrintTaints;
  bool _shouldWriteErrors;
  AnalysisState _analysisState;

public:
  FunctionProcessor(TaintFlowPass& pass, const Function& f, CircleMap& circRef, const Module& m, const Logger& logger)
  : F(f), DT(pass.getDependency<DominatorTree>(f)), PDT(pass.getDependency<PostDominatorTree>(f)), M(m),
    PASS(pass), _circularReferences(circRef), logger(logger), setHelper(logger)
  {
    _suppressPrintTaints = false;
    _shouldWriteErrors = true;

    DOT = new NullGraphExporter();

    DEBUG(delete DOT);
    DEBUG(DOT = new GraphExporter(f.getName()));

    CTX = new InstructionHandlerContext(*DOT, DT, PDT, logger, _workList, _analysisState, f, _circularReferences, setHelper, pass, m),
    BH = new BlockHelper(DT, PDT, *DOT, logger);
    IHD = new InstructionHandlerDispatcher(*CTX);

    registerHandlers();
  }

  ~FunctionProcessor() {
    delete DOT;
    delete BH;
    delete IHD;
    delete CTX;
  }

  void processFunction();


  AnalysisState getAnalysisState() {
    return _analysisState;
  }

  bool didFinish() {
    return !_analysisState.isCanceled();
  }

  void setShouldWriteErrors(bool val) {
    _shouldWriteErrors = val;
  }

  void suppressPrintTaints() {
      _suppressPrintTaints = true;
  }

  ResultSet getResult() {
      return setHelper.resultSet;
  }

private:
  void buildTaintSetFor(const Value& arg, TaintSet& taintSet);
  void addTaint(const Value& tainter, const Value& taintee);
  void processBasicBlock(const BasicBlock& block, TaintSet& taintSet);
  void printTaints();

  void handleBlockTainting(const Instruction& inst, const BasicBlock& currentblock, TaintSet& taintSet);

  void findArguments();
  void handleFoundArgument(const Value& arg);
  void findAllStoresAndLoadsForOutArgumentAndAddToSet(const Value& arg, ReturnSet& retlist);
  void findReturnStatements();
  void printInstructions(); 
  void recursivelyFindAliases(const Value& arg, ReturnSet& taintSet, ReturnSet& alreadyProcessed);
  void applyMeet(const BasicBlock& block);

  void registerHandlers() {
    IHD->registerDefaultHandler<DefaultHandler>();
    IHD->registerHandler<GetElementPtrHandler>();
    IHD->registerHandler<PhiNodeHandler>();
    IHD->registerHandler<BranchHandler>();
    IHD->registerHandler<SwitchHandler>();
    IHD->registerHandler<StoreHandler>();
    IHD->registerHandler<IndirectBranchHandler>();
    IHD->registerHandler<CallHandler>();
  }

};

#endif // FUNCTION_PROCESSOR_H
