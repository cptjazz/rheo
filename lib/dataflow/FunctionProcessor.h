#ifndef FUNCTION_PROCESSOR_H
#define FUNCTION_PROCESSOR_H

#include "Logger.h"
#include "GraphExporter.h"
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
#include "IntToPtrHandler.h"
#include "CallHandler.h"
#include "DefaultHandler.h"
#include "Logger.h"
#include "SetHelper.h"
#include "ExcludeFile.h"
#include "SpecialTaintHelper.h"
#include "FopenSpecialTaintInstruction.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <deque>
#include <cstring>
#include <stdio.h>

using namespace llvm;
using namespace std;


class FunctionProcessor {
  const Function& F;
  DominatorTree& DT;
  PostDominatorTree& PDT;
  const Module& M;
  TaintFlowPass& PASS;
  CircleMap& _circularReferences;
  const Logger& logger;
  SetHelper setHelper;
  GraphExporter DOT;
  SpecialTaintHelper& STH;
  InstructionHandlerContext CTX;
  InstructionHandlerDispatcher IHD;
  BlockHelper BH;
  AnalysisState _analysisState;

  map<const BasicBlock*, TaintSet> _blockList;
  deque<const BasicBlock*> _workList;

  bool _suppressPrintTaints;

public:
  FunctionProcessor(TaintFlowPass& pass, const Function& f, CircleMap& circRef, const Module& m, const Logger& logger, ExcludeFile& exclusions, SpecialTaintHelper& sth)
  : F(f), DT(pass.getDependency<DominatorTree>(f)), PDT(pass.getDependency<PostDominatorTree>(f)), M(m),
    PASS(pass), _circularReferences(circRef), logger(logger), setHelper(logger), DOT(f.getName()), STH(sth),
    CTX(DOT, DT, PDT, logger, _workList, _analysisState, f, _circularReferences, setHelper, pass, m, exclusions, STH),
    IHD(CTX), BH(DT, PDT, DOT, logger), _analysisState(logger)
   {
    _suppressPrintTaints = false;

    IF_GRAPH(DOT.init());
    registerHandlers();
  }

  void processFunction();

  static FunctionProcessor& from(InstructionHandlerContext& ctx, const Function& func) {
    return *new FunctionProcessor(ctx.PASS, func, ctx.circularReferences, ctx.M, ctx.logger, ctx.EXCL, ctx.STH);
  }

  AnalysisState getAnalysisState() {
    return _analysisState;
  }

  bool didFinish() {
    return !_analysisState.isCanceled();
  }

  void suppressPrintTaints() {
    _suppressPrintTaints = true;
  }

  ResultSet getResult() {
    return setHelper.resultSet;
  }

private:
  void buildTaintSetFor(const Value& arg, TaintSet& taintSet, TaintSet& initialTaints);
  void addTaint(const Value& tainter, const Value& taintee);
  void processBasicBlock(const BasicBlock& block, TaintSet& taintSet);
  void printTaints();
  void handleBlockTainting(const Instruction& inst, const BasicBlock& currentblock, TaintSet& taintSet);
  void findArguments();
  void handleFoundArgument(TaintType taintType, const Value& arg, const TaintSet& initialValues = TaintSet());
  void findReturnStatements();
  void printInstructions(); 
  void applyMeet(const BasicBlock& block);

  void registerHandlers() {
    IHD.registerDefaultHandler<DefaultHandler>();
    IHD.registerHandler<GetElementPtrHandler>();
    IHD.registerHandler<PhiNodeHandler>();
    IHD.registerHandler<BranchHandler>();
    IHD.registerHandler<SwitchHandler>();
    IHD.registerHandler<StoreHandler>();
    IHD.registerHandler<CallHandler>();

    IHD.registerHandlerForUnsupportedInstruction<IndirectBranchHandler>();
    IHD.registerHandlerForUnsupportedInstruction<IntToPtrHandler>();

    STH.registerFunction<FopenSpecialTaintInstruction>();
  }

};

#endif // FUNCTION_PROCESSOR_H
