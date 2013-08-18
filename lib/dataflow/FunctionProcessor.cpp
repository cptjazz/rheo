#include "FunctionProcessor.h"
#include "TaintSet.h"
#include "SpecialTaintHelper.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/Casting.h"
#include "llvm/DebugInfo.h"
#include "llvm/IR/Intrinsics.h"
#include <map>
#include <set>
#include <algorithm>
#include <stdio.h>


#define STOP_ON_CANCEL if (_analysisState.isCanceled()) return


const SpecialTaint& SpecialTaint::Null = SpecialTaint(NULL, NoTaint);

void FunctionProcessor::processFunction() {
  DEBUG(printInstructions());

  logger.debug() << "Spawning analysis for: " << F.getName() << "\n";
  findReturnStatements();
  findArguments();

  int resultIteration = 0;

  const size_t argCount = setHelper.arguments.size();
  if (!_suppressPrintTaints)
    logger.info() << "arg_count:" << argCount << "\n";

  std::map<const Value*, TaintSet> initialTaintMap;

  TaintMap::iterator arg_i = setHelper.arguments.begin();
  TaintMap::iterator arg_e = setHelper.arguments.end();

  for(; arg_i != arg_e; ++arg_i) {
    initialTaintMap.insert(std::make_pair(arg_i->first, arg_i->second));
  }

  do {
    setHelper.resetResultSetChanged();

    DEBUG(logger.debug() << "Starting arg iteration " << resultIteration << " for " << F.getName() << "\n");

    TaintMap::iterator arg_i = setHelper.arguments.begin();
    TaintMap::iterator arg_e = setHelper.arguments.end();
      
    size_t argIdx = 0;

    for(; arg_i != arg_e; ++arg_i) {
      argIdx++;

      const Value& arg = *arg_i->first;
      TaintSet& taintSet = arg_i->second;

      CTX.currentArgument = &arg;
      taintSet.resetChangedFlag();

      if (!_suppressPrintTaints)
        logger.info() << "arg_no:" << argIdx << "\n";

      buildTaintSetFor(arg, taintSet, initialTaintMap[&arg]);
      STOP_ON_CANCEL;
    }

    resultIteration++;
    setHelper.buildResultSet();
  } while (resultIteration < 10 && setHelper.hasResultSetChanged());

  _analysisState.setProcessingState(Success);

  if (!_suppressPrintTaints)
    printTaints();
}



void FunctionProcessor::buildTaintSetFor(const Value& arg, TaintSet& taintSet, TaintSet& initialTaints) {
  DEBUG(logger.debug() << " *** Creating taint set for argument `" << Helper::getValueName(arg) << "`\n");

  _blockList.clear();
  _workList.clear();

  for (Function::const_iterator b_i = F.begin(), b_e = F.end(); b_i != b_e; ++b_i) {
    const BasicBlock& block = cast<BasicBlock>(*b_i);
    TaintSet blockTaintSet;
    _blockList.insert(std::make_pair(&block, blockTaintSet));
    _workList.push_back(&block);
  }

  const BasicBlock* firstBlock = _workList.front();
  const BasicBlock* lastBlock = _workList.back();

  // Initialize the first block with the 
  // taints of the currently inspected argument.
  _blockList[firstBlock].addAll(initialTaints);


  do {
    STOP_ON_CANCEL;

    taintSet.resetChangedFlag();

    while (!_workList.empty()) {
      const BasicBlock& block = *_workList.front();
      _workList.pop_front();

      DEBUG(logger.debug() << " ----- PROCESS BLOCK " << block.getName() << " -----\n");
      applyMeet(block);

      processBasicBlock(block, _blockList[&block]);
      STOP_ON_CANCEL;
    }

  } while(taintSet.hasChanged());


  // The last block represents the result.
  // Every taint flows were propagated to this
  // block due to the meet-operation
  taintSet.clear();
  taintSet.addAll(_blockList[lastBlock]);

  DEBUG(logger.debug() << "Taint set for arg `" << arg.getName() << " (" << &arg << ")`:\n");
  DEBUG(taintSet.printTo(logger.debug()));
}

/**
 * Apply the meeting operation (union) for this basic block.
 *
 *  set_current = UNION_i=predecessor (s_i)
 */
void FunctionProcessor::applyMeet(const BasicBlock& block) {
  TaintSet& blockSet = _blockList[&block];
  DEBUG(logger.debug() << "Applying meet for: " << block.getName() << "\n");
  IF_PROFILING(long t = Helper::getTimestamp());
  
  for (const_pred_iterator i = pred_begin(&block), e = pred_end(&block); i != e; ++i) {
    const BasicBlock& pred = **i;
    DEBUG(logger.debug() << "Meeting block: " << pred.getName() << "\n");

    TaintSet& predSet = _blockList[&pred];
    blockSet.addAll(predSet);
  }

  IF_PROFILING(logger.profile() << "Meeting took " << Helper::getTimestampDelta(t) << "µs\n");
  DEBUG(logger.debug() << "End meet\n");
}


void FunctionProcessor::processBasicBlock(const BasicBlock& block, TaintSet& taintSet) {
  bool blockTainted = taintSet.contains(block) || BH.isBlockTaintedByOtherBlock(block, taintSet);

  for (BasicBlock::const_iterator inst_i = block.begin(), inst_e = block.end(); inst_i != inst_e; ++inst_i) {
    STOP_ON_CANCEL;

    IF_PROFILING(long t = Helper::getTimestamp());

    const Instruction& inst = cast<Instruction>(*inst_i);

    if (blockTainted)
      handleBlockTainting(inst, block, taintSet);

    DEBUG(CTX.logger.debug() << "`` Taint Set before dispatch:\n");
    DEBUG(taintSet.printTo(CTX.logger.debug()));

    DEBUG(CTX.logger.debug() << "::Inst:: " << inst << "\n");
    IHD.dispatch(inst, taintSet);

    DEBUG(CTX.logger.debug() << "`` Taint Set after dispatch:\n");
    DEBUG(taintSet.printTo(CTX.logger.debug()));

    IF_PROFILING(logger.profile() << " Processing instruction '" << Instruction::getOpcodeName(inst.getOpcode())
        << "' took " << Helper::getTimestampDelta(t) << " µs\n");
  }
}


void FunctionProcessor::printTaints() {
  logger.output() << "__taints:";
  logger.output() << F.getName() << "(";
  bool isFirstTime = true;

  for (ResultSet::const_iterator i = setHelper.resultSet.begin(), e = setHelper.resultSet.end(); i != e; ++i) {
    const Value& arg = cast<Value>(*i->first);
    const Value& retval = cast<Value>(*i->second);

    logger.output() << (isFirstTime ? "" : ", ") << Helper::getValueName(arg) << " => " << Helper::getValueName(retval);
    isFirstTime = false;
  }

  logger.output() << ")\n";
}


void FunctionProcessor::handleBlockTainting(const Instruction& inst, const BasicBlock& currentBlock, TaintSet& taintSet) {
  DEBUG(logger.debug() << " Handle BLOCK-tainting for `" << inst << "`\n");

  // Loads should not be tained by parenting block
  // because otherwise a block would taint a load of
  // a global variable what makes no sense -- it would
  // introduce a taint that does not exist.
  if (isa<LoadInst>(inst)) {
    DEBUG(logger.debug() << " Ignoring LOAD\n");
    return;
  }

  if (isa<GetElementPtrInst>(inst)) {
    DEBUG(logger.debug() << " Ignoring GEP\n");
    return;
  }

  if (isa<BranchInst>(inst) && (cast<BranchInst>(inst)).isUnconditional())
    return;

  taintSet.add(inst);
  DEBUG(logger.debug() << " + Instruction tainted by dirty block: " << inst << "\n");

  if (isa<StoreInst>(inst))
    IF_GRAPH(DOT.addRelation(currentBlock, *inst.getOperand(0), "block-taint"));
  else
    IF_GRAPH(DOT.addRelation(currentBlock, inst, "block-taint"));
}

void FunctionProcessor::findArguments() {
  // Special taint sources/sinks
  //
  for (const_inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    const Value& v = *i;
    const CallInst* call = dyn_cast<CallInst>(&v);

    if (!call)
      continue;

    const SpecialTaint newTaint = STH.getExternalTaints(*call);

    if (newTaint.type != NoTaint)
      handleFoundArgument(newTaint.type, *(newTaint.specialTaintValue));

    std::set<SpecialTaint>& set = STH.getCalledFunctionSpecialTaints(call->getCalledFunction());
    for (std::set<SpecialTaint>::iterator s_i = set.begin(), s_e = set.end(); s_i != s_e; ++s_i) {
      const SpecialTaint taint = *s_i;
      handleFoundArgument(taint.type, *(taint.specialTaintValue));
    }
  }

  // Ordinary function arguments
  //
  for (Function::const_arg_iterator a_i = F.arg_begin(), a_e = F.arg_end(); a_i != a_e; ++a_i) {
    const Argument& arg = *a_i;

    TaintType taintType = (arg.getType()->isPointerTy())
      ? Source | Sink
      : Source;

    handleFoundArgument(taintType, arg);
  }

  // Global variables
  //
  for (Module::const_global_iterator g_i = M.global_begin(), g_e = M.global_end(); g_i != g_e; ++g_i) {
    const GlobalVariable& g = *g_i;

    // Skip constants (eg. string literals)
    if (g.isConstant())
      continue;

    handleFoundArgument(Source | Sink, g);
  }

  // Varargs handling
  //
  if (!F.isVarArg())
    return;

  // In a perfect world, compilers would use the LLVM va_arg instruction
  // to copy over the current vararg-element. But neither Clang nor gcc-llvm
  // use this instruction, instead they immediately lower the code to use
  // some struct magic.
  // Here, we search for this struct and rename it to "..." to have convenient
  // output. We simply search for the first @va_start intrinsic and follow its
  // argument until we reach the struct.
  for (const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
    const Value& v = *I;
    const CallInst* call = dyn_cast<CallInst>(&v);

    if (!call)
      continue;

    if (!call->getCalledFunction())
      continue;

    const Function& calledFunction = *call->getCalledFunction();
    if (calledFunction.isIntrinsic() && calledFunction.getIntrinsicID() == Intrinsic::vastart) {
      // Found Value of var-arg list.
      Value* alloca;

      const BitCastInst* bitcast = cast<BitCastInst>(call->getOperand(0));

      // Depending on -instcombine, there may (or may not) be a GEP 
      // between the bitcast and the alloca.
      if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(bitcast->getOperand(0)))
        alloca = gep->getOperand(0);
      else 
        alloca = bitcast->getOperand(0);

      alloca->setName("...");
      handleFoundArgument(Source | Sink, *alloca);
      break;
    }
  }
}

void FunctionProcessor::handleFoundArgument(TaintType taintType, const Value& arg, const TaintSet& initialValues) {

  DEBUG(logger.debug() << " -- Inspecting argument or global `" << Helper::getValueName(arg) << "`\n");

  if (taintType & Sink) {
    TaintSet returnSet;
    returnSet.add(arg);

    setHelper.returnStatements.insert(std::make_pair(&arg, returnSet));

    IF_GRAPH(DOT.addInOutNode(arg));
    DEBUG(logger.debug() << "added arg `" << Helper::getValueName(arg) << "` to out-list\n");
  } else {
    IF_GRAPH(DOT.addInNode(arg));
  }

  TaintSet taintSet;
  taintSet.add(arg);

  for (TaintSet::const_iterator i_i = initialValues.begin(), i_e = initialValues.end(); i_i != i_e; ++i_i) {
    const Value& v = **i_i;
    IF_GRAPH(DOT.addRelation(arg, v, "created"));
    taintSet.add(v);
  }

  setHelper.arguments.insert(std::make_pair(&arg, taintSet));
  DEBUG(logger.debug() << "added arg `" << Helper::getValueName(arg) << "` to arg-list\n");
}

void FunctionProcessor::findReturnStatements() {
  for (const_inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    const Value& v = *i;
    const ReturnInst* r = dyn_cast<ReturnInst>(&v);

    if (!r) 
      continue;

    TaintSet taintSet;

    // skip 'return void'
    const Value* retval = r->getReturnValue();
    if (retval) {
      if (isa<Constant>(retval)) {
        taintSet.add(*r);
        DEBUG(logger.debug() << " + Added instruction CONSTANT RETURN VALUE `" << *retval << "`\n");
      } else {
        taintSet.add(*retval);
        DEBUG(logger.debug() << " + Added NON-CONST RETURN VALUE `" << retval << "`\n");
      }

      setHelper.returnStatements.insert(std::make_pair(r, taintSet));
      IF_GRAPH(DOT.addOutNode(*r));
      DEBUG(logger.debug() << "Found ret-stmt: " << *r << "\n");
    }
  }
}

void FunctionProcessor::printInstructions() {
  logger.debug() << "Instructions: \n";

  for (const_inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    if (MDNode *n = i->getMetadata("dbg")) {  // Here I is an LLVM instruction
      DILocation loc(n);                      // DILocation is in DebugInfo.h
      unsigned int line = loc.getLineNumber();
      StringRef file = loc.getFilename();
      logger.debug() << file << ":" << line << " | ";
    }

    logger.debug() << i->getParent() << " | " << &*i << " | " << (*i) << "\n";
  }
}
