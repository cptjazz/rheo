#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/GlobalVariable.h"
#include "llvm/InstrTypes.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/Casting.h"
#include "llvm/Intrinsics.h"
#include <map>
#include <set>
#include <algorithm>
#include <stdio.h>
#include "FunctionProcessor.h"
#include "TaintSet.h"


#define STOP_ON_CANCEL if (_analysisState.isCanceled()) return
#define ERROR_LOG(x) if (_shouldWriteErrors) _stream << "__error:" << x 

using namespace llvm;
using namespace std;


void FunctionProcessor::processFunction() {
  DEBUG(printInstructions());

  findReturnStatements();
  findArguments();

  int resultIteration = 0;

  do {
    setHelper.resetResultSetChanged();

    DEBUG(logger.debug() << "Starting arg iteration " << resultIteration << " for " << F.getName() << "\n");

    TaintMap::iterator arg_i = setHelper.arguments.begin();
    TaintMap::iterator arg_e = setHelper.arguments.end();
      
    for(; arg_i != arg_e; ++arg_i) {
      const Value& arg = *arg_i->first;
      TaintSet& taintSet = arg_i->second;

      taintSet.resetChangedFlag();

      buildTaintSetFor(arg, taintSet);
      STOP_ON_CANCEL;
    }

    resultIteration++;
    setHelper.buildResultSet();
  } while (resultIteration < 10 && setHelper.hasResultSetChanged());

  _analysisState.setProcessingState(Success);

  if (!_suppressPrintTaints)
    printTaints();
}



void FunctionProcessor::buildTaintSetFor(const Value& arg, TaintSet& taintSet) {
  DEBUG(logger.debug() << " *** Creating taint set for argument `" << arg.getName() << "`\n");

  // Arg trivially taints itself.
  taintSet.add(arg);

  _blockList.clear();
  _workList.clear();

  for (Function::const_iterator b_i = F.begin(), b_e = F.end(); b_i != b_e; ++b_i) {
    const BasicBlock& block = cast<BasicBlock>(*b_i);
    TaintSet blockTaintSet;
    blockTaintSet.add(arg);
    _blockList.insert(make_pair(&block, blockTaintSet));
    _workList.push_back(&block);
  }

  do {
    STOP_ON_CANCEL;

    taintSet.resetChangedFlag();

    while (!_workList.empty()) {
      const BasicBlock& block = *_workList.front();
      _workList.pop_front();

      DEBUG(logger.debug() << " ----- PROCESS BLOCK " << block.getName() << " -----\n");
      applyMeet(block);

      processBasicBlock(block, _blockList[&block]);
    }

  } while(taintSet.hasChanged());

  // Merge all par-block taint-sets
  for (map<const BasicBlock*, TaintSet>::const_iterator j_i = _blockList.begin(), j_e = _blockList.end(); j_i != j_e; ++j_i) {
    TaintSet& set = _blockList[j_i->first];
    taintSet.addAll(set);
  }

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
  bool blockTainted = taintSet.contains(block) || BH->isBlockTaintedByOtherBlock(block, taintSet);

  for (BasicBlock::const_iterator inst_i = block.begin(), inst_e = block.end(); inst_i != inst_e; ++inst_i) {
    STOP_ON_CANCEL;

    IF_PROFILING(long t = Helper::getTimestamp());

    Instruction& inst = cast<Instruction>(*inst_i);

    if (blockTainted)
      handleBlockTainting(inst, block, taintSet);

    IHD->dispatch(inst, taintSet);

    IF_PROFILING(logger.profile() << " Processing instruction '" << Instruction::getOpcodeName(inst.getOpcode())
        << "' took " << Helper::getTimestampDelta(t) << " µs\n");
  }
}


void FunctionProcessor::printTaints() {
  logger.output() << "__taints:";
  logger.output() << F.getName() << "(";
  bool isFirstTime = true;

  for (ResultSet::const_iterator i = setHelper.resultSet.begin(), e = setHelper.resultSet.end(); i != e; ++i) {
    Value& arg = cast<Value>(*i->first);
    Value& retval = cast<Value>(*i->second);

    logger.output() << (isFirstTime ? "" : ", ") << arg.getName() << " => " << Helper::getValueNameOrDefault(retval);
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

  taintSet.add(inst);
  DEBUG(logger.debug() << " + Instruction tainted by dirty block: " << inst << "\n");

  if (isa<StoreInst>(inst))
    DEBUG(DOT->addRelation(currentBlock, *inst.getOperand(0), "block-taint"));
  else
    DEBUG(DOT->addRelation(currentBlock, inst, "block-taint"));
}

void FunctionProcessor::findArguments() {
  for (Function::const_arg_iterator a_i = F.arg_begin(), a_e = F.arg_end(); a_i != a_e; ++a_i) {
    handleFoundArgument(*a_i);
  }

  for (Module::const_global_iterator g_i = M.global_begin(), g_e = M.global_end(); g_i != g_e; ++g_i) {
    // Skip constants (eg. string literals)
    if (!g_i->isConstant())
      handleFoundArgument(*g_i);
  }

  // In a perfect world, compilers would use the LLVM va_arg instruction
  // to copy over the current vararg-element. But neither Clang nor gcc-llvm
  // use this instruction, instead they immediately lower the code to use
  // some struct magic.
  // Here, we search for this struct and rename it to "..." to have convenient
  // output. We simply search for the first @va_start intrinsic and follow its
  // argument until we reach the struct.
  if (F.isVarArg()) {
    for (const_inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      if (isa<CallInst>(*I)) {
        const CallInst& call = cast<CallInst>(*I);

        if (!call.getCalledFunction())
          continue;

        if (call.getCalledFunction()->isIntrinsic() && call.getCalledFunction()->getIntrinsicID() == Intrinsic::vastart) {
          // Found Value of var-arg list.
          BitCastInst& bitcast = cast<BitCastInst>(*call.getOperand(0));
          GetElementPtrInst& gep = cast<GetElementPtrInst>(*bitcast.getOperand(0));
          Value& alloca = *gep.getOperand(0);

          alloca.setName("...");
          handleFoundArgument(alloca);
          break;
        }
      }
    }
  }
}

void FunctionProcessor::handleFoundArgument(const Value& arg) {
  bool isInOutNode = false;

  logger.debug() << " -- Inspecting argument or global `" << arg.getName() << "`\n";

  if ((arg.getType()->isPointerTy() || isa<GlobalVariable>(arg))) {
    TaintSet returnSet;
    returnSet.add(arg);

    findAllStoresAndLoadsForOutArgumentAndAddToSet(arg, returnSet);

    setHelper.returnStatements.insert(make_pair(&arg, returnSet));
    DOT->addInOutNode(arg);
    isInOutNode = true;

    logger.debug() << "added arg `" << arg.getName() << "` to out-list\n";
  }

  TaintSet taintSet;
  taintSet.add(arg);
  if (!isInOutNode)
    DOT->addInNode(arg);

  setHelper.arguments.insert(make_pair(&arg, taintSet));
  logger.debug() << "added arg `" << arg.getName() << "` to arg-list\n";
}

void FunctionProcessor::findAllStoresAndLoadsForOutArgumentAndAddToSet(const Value& arg, ReturnSet& returnSet) {
    const Value* newArg = &arg;

    for (User::const_use_iterator u_i = arg.use_begin(), u_e = arg.use_end(); u_i != u_e; ++u_i) {
      if (isa<StoreInst>(*u_i)) {
        returnSet.add(**u_i);
        DEBUG(logger.debug() << " Added ARG STORE: " << **u_i << "\n");
        newArg = u_i->getOperand(1);

        ReturnSet alreadyProcessed;
        recursivelyFindAliases(*newArg, returnSet, alreadyProcessed);
      }
    }
}

void FunctionProcessor::recursivelyFindAliases(const Value& arg, ReturnSet& returnSet, ReturnSet& alreadyProcessed) {

  DEBUG(logger.debug() << "Recursively find: " << arg << "\n");

  if (alreadyProcessed.contains(arg))
    return;

  alreadyProcessed.add(arg);

  for (User::const_use_iterator i = arg.use_begin(), e = arg.use_end(); i != e; ++i) {
    if (!isa<Instruction>(**i)) {
      // Necessary, as constant string literals also come 
      // as GlobalVariables, but do not point to an instruction.
      // They point to an operand of a GEP.
      DEBUG(logger.debug() << "Skip: " << **i << "\n");
      continue;
    }

    const Instruction& I = cast<Instruction>(**i);
    
    DEBUG(logger.debug() << "Inspecting: " << I << "\n");

    if (isa<LoadInst>(I) && I.getOperand(0) == &arg) {
      const Value& load = cast<LoadInst>(I);
      returnSet.add(load);
      DEBUG(DOT->addRelation(arg, load, "load"));
      DEBUG(logger.debug() << " + Found LOAD for `" << arg.getName() << "` @ " << load << "\n");
      
      recursivelyFindAliases(load, returnSet, alreadyProcessed);
    }
  }
}

void FunctionProcessor::findReturnStatements() {
  for (const_inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    if (isa<ReturnInst>(*i)) {
      const ReturnInst& r = cast<ReturnInst>(*i);
      TaintSet taintSet;
      
      // skip 'return void'
      const Value* retval = r.getReturnValue();
      if (retval) {
        if (isa<Constant>(retval)) {
          taintSet.add(r);
          DEBUG(logger.debug() << " + Added instruction CONSTANT RETURN VALUE `" << *retval << "`\n");
        } else {
          taintSet.add(*retval);
          DEBUG(logger.debug() << " + Added NON-CONST RETURN VALUE `" << retval << "`\n");
        }

        setHelper.returnStatements.insert(make_pair(&r, taintSet));
        DEBUG(DOT->addOutNode(r));
        DEBUG(logger.debug() << "Found ret-stmt: " << r << "\n");
      }
    }
  }
}

void FunctionProcessor::printInstructions() {
  logger.debug() << "Instructions: \n";

  for (const_inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    logger.debug() << i->getParent() << " | " << &*i << " | " << (*i) << "\n";
  }
}
