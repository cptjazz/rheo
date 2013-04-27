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
#include <cstring>
#include <sstream>
#include <stdio.h>
#include "FunctionProcessor.h"
#include "TaintFile.h" 
#include "TaintSet.h" 
#include "IntrinsicHelper.h" 


#define STOP_ON_CANCEL if (_canceledInspection) return
#define DEBUG_LOG(x) DEBUG(debug() << x)
#define ERROR_LOG(x) if (_shouldWriteErrors) debug() << "__error:" << x 
#ifdef PROFILING
#define PROFILE_LOG(x) DEBUG(debug() << x)
#define IF_PROFILING(x) x
#else
#define PROFILE_LOG(x) 
#define IF_PROFILING(x)
#endif

using namespace llvm;
using namespace std;


void FunctionProcessor::processFunction() {
  printInstructions();

  findReturnStatements();
  findArguments();

  int resultIteration = 0;

  do {
    _resultSetChanged = false;

    DEBUG_LOG("Starting arg iteration " << resultIteration << " for " << F.getName() << "\n");

    TaintMap::iterator arg_i = _arguments.begin();
    TaintMap::iterator arg_e = _arguments.end();
      
    for(; arg_i != arg_e; ++arg_i) {
      const Value& arg = *arg_i->first;
      TaintSet& taintSet = arg_i->second;

      taintSet.resetChangedFlag();

      buildTaintSetFor(arg, taintSet);
      STOP_ON_CANCEL;
    }

    resultIteration++;
    buildResultSet(true);
  } while (resultIteration < 10 && _resultSetChanged);

  _processingState = Success;

  if (!_suppressPrintTaints)
    printTaints();
}

void FunctionProcessor::buildResultSet(const bool debugPrintSet) {

  TaintMap::const_iterator arg_i = _arguments.begin();
  TaintMap::const_iterator arg_e = _arguments.end();
      
  for(; arg_i != arg_e; ++arg_i) {
    const Value& arg = *arg_i->first;
    const TaintSet& taintSet = arg_i->second;

    intersectSets(arg, taintSet, debugPrintSet);
  }
}

void FunctionProcessor::intersectSets(const Value& arg, const TaintSet argTaintSet, const bool debugPrintSet) {
  TaintMap::const_iterator ret_i = _returnStatements.begin();
  TaintMap::const_iterator ret_e = _returnStatements.end();

  for (; ret_i != ret_e; ++ret_i) {
    const Value& retval = *ret_i->first;
    const TaintSet retTaintSet = ret_i->second;

    if (&retval == &arg) {
      DEBUG_LOG("Skipping detected self-taint\n");
      continue;
    }

    IF_PROFILING(long t = Helper::getTimestamp());
    DEBUG_LOG("Ret-set for `" << retval << "`:\n");
    if (debugPrintSet)
      retTaintSet.printTo(_stream);

    IF_PROFILING(t = Helper::getTimestamp());
    TaintSet intersect;
    argTaintSet.intersect(retTaintSet, intersect);
    PROFILE_LOG("intersect() took " << Helper::getTimestampDelta(t) << " µs\n");

    if (intersect.size()) {
      addTaint(arg, retval);

      DEBUG_LOG("Values that lead to taint " << Helper::getValueNameOrDefault(arg) << " -> "
              << Helper::getValueNameOrDefault(retval) << ":\n");
      intersect.printTo(_stream);
    }
  }
}


void FunctionProcessor::buildTaintSetFor(const Value& arg, TaintSet& taintSet) {
  DEBUG_LOG(" *** Creating taint set for argument `" << arg.getName() << "`\n");

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

      DEBUG_LOG(" ----- PROCESS BLOCK " << block.getName() << " -----\n");
      applyMeet(block);

      processBasicBlock(block, _blockList[&block]);
    }

  } while(taintSet.hasChanged());

  for (map<const BasicBlock*, TaintSet>::const_iterator j_i = _blockList.begin(), j_e = _blockList.end(); j_i != j_e; ++j_i) {
    TaintSet& set = _blockList[j_i->first];
    taintSet.addAll(set);
  }

  DEBUG_LOG("Taint set for arg `" << arg.getName() << " (" << &arg << ")`:\n");
  taintSet.printTo(_stream);
}

/**
 * Apply the meeting operation (union) for this basic block.
 *
 *  set_current = UNION_i=predecessor (s_i)
 */
void FunctionProcessor::applyMeet(const BasicBlock& block) {
  TaintSet& blockSet = _blockList[&block];
  DEBUG_LOG("Applying meet for: " << block.getName() << "\n");
  
  for (const_pred_iterator i = pred_begin(&block), e = pred_end(&block); i != e; ++i) {
    const BasicBlock& pred = **i;
    DEBUG_LOG("Meeting block: " << pred.getName() << "\n");

    TaintSet& predSet = _blockList[&pred];
    blockSet.addAll(predSet);
  }

  DEBUG_LOG("End meet\n");
}

/**
 * Adds a taint to the final result set.
 * Also switches the _resultSetChanged flag if the taint is new.
 *
 * @param tainter The source of the taint (usually an argument)
 * @param taintee The sink of the taint (usually a return or out-pointer)
 */
inline void FunctionProcessor::addTaint(const Value& tainter, const Value& taintee) {
  if (_taints.insert(make_pair(&tainter, &taintee)).second) {
    _resultSetChanged = true;
    DEBUG_LOG("Added taint. Result set changed.\n");
  }
}

void FunctionProcessor::processBasicBlock(const BasicBlock& block, TaintSet& taintSet) {
  bool blockTainted = isBlockTaintedByOtherBlock(block, taintSet);

  for (BasicBlock::const_iterator inst_i = block.begin(), inst_e = block.end(); inst_i != inst_e; ++inst_i) {
    STOP_ON_CANCEL;

    IF_PROFILING(long t = Helper::getTimestamp());

    Instruction& inst = cast<Instruction>(*inst_i);

    if (blockTainted)
      handleBlockTainting(inst, block, taintSet);

    if (isa<BranchInst>(inst))
      handleBranchInstruction(cast<BranchInst>(inst), taintSet);
    if (isa<PHINode>(inst))
      handlePhiNode(cast<PHINode>(inst), taintSet);
    else if (isa<StoreInst>(inst))
      handleStoreInstruction(cast<StoreInst>(inst), taintSet);
    else if (isa<CallInst>(inst))
      handleCallInstruction(cast<CallInst>(inst), taintSet);
    else if (isa<GetElementPtrInst>(inst))
      handleGetElementPtrInstruction(cast<GetElementPtrInst>(inst), taintSet);
    else if (isa<SwitchInst>(inst))
      handleSwitchInstruction(cast<SwitchInst>(inst), taintSet);
    else
      handleInstruction(inst, taintSet);

    PROFILE_LOG(" Processing instruction '" << Instruction::getOpcodeName(inst.getOpcode()) 
        << "' took " << Helper::getTimestampDelta(t) << " µs\n");
  }
}

void FunctionProcessor::printTaints() {
  release() << "__taints:";
  release().write_escaped(F.getName()) << "(";
  bool isFirstTime = true;

  for (ResultSet::const_iterator i = _taints.begin(), e = _taints.end(); i != e; ++i) {
    Value& arg = cast<Value>(*i->first);
    Value& retval = cast<Value>(*i->second);

    release() << (isFirstTime ? "" : ", ") << arg.getName() << " => " << Helper::getValueNameOrDefault(retval);
    isFirstTime = false;
  }

  release() << ")\n";
}

void FunctionProcessor::handleGetElementPtrInstruction(const GetElementPtrInst& inst, TaintSet& taintSet) {
  const Value& op = *inst.getPointerOperand();

  if (taintSet.contains(op)) {
    taintSet.add(inst);
    DOT->addRelation(op, inst, "indexer");
    DEBUG_LOG(" + Added GEP taint: `" << inst << "`\n");
  }
  
  for (size_t i = 0; i < inst.getNumIndices(); i++) {
    const Value& idx = *inst.getOperand(i + 1);

    if (taintSet.contains(idx)) {
      taintSet.add(inst);
      stringstream reason("");
      reason << "index #" << i;
      DOT->addRelation(idx, inst, reason.str());
      DEBUG_LOG(" ++ Added GEP INDEX: `" << idx << "`\n");
    }
  }
}

void FunctionProcessor::handleStoreInstruction(const StoreInst& storeInst, TaintSet& taintSet) {
  const Value& source = *storeInst.getOperand(0);
  const Value& target = *storeInst.getOperand(1);

  DEBUG_LOG(" Handle STORE instruction " << storeInst << "\n");
  if (taintSet.contains(source) || taintSet.contains(storeInst)) {
    taintSet.add(target);
    taintSet.add(storeInst);

    DOT->addRelation(source, target, "store");
    DEBUG_LOG(" + Added STORE taint: " << source << " --> " << target << "\n");
    if (isa<GetElementPtrInst>(target) || isa<LoadInst>(target)) {
      const Instruction& inst = cast<Instruction>(target);
      recursivelyAddAllGepsAndLoads(inst, taintSet);
    }

  } else if (taintSet.contains(target) && isCfgSuccessorOfPreviousStores(storeInst, taintSet)) {
    // Only do removal if value is really in set
    taintSet.remove(target);
    DEBUG_LOG(" - Removed STORE taint due to non-tainted overwrite: " << source << " --> " << target << "\n");

    if (isa<LoadInst>(target)) {
      const Value& load = *(cast<LoadInst>(target)).getPointerOperand();
      taintSet.remove(load);
      DEBUG_LOG(" - Also removed transitive LOAD." << load << "\n");
    }

    DOT->addRelation(source, target, "non-taint overwrite");
  }
}

void FunctionProcessor::recursivelyAddAllGepsAndLoads(const Instruction& target, TaintSet& taintSet) {
  if (isa<GetElementPtrInst>(target)) {
    const GetElementPtrInst& gep = cast<GetElementPtrInst>(target);
    const Value& ptrOp = *gep.getPointerOperand();
    DEBUG_LOG(" ++ Added GEP SOURCE:" << ptrOp << "\n");
    taintSet.add(ptrOp);

    DOT->addRelation(gep, ptrOp, "gep via store");

    if (isa<Instruction>(ptrOp))
      recursivelyAddAllGepsAndLoads(cast<Instruction>(ptrOp), taintSet);
  } else if (isa<LoadInst>(target)) {
    const LoadInst& load = cast<LoadInst>(target);
    const Value& ptrOp = *load.getOperand(0);
    DEBUG_LOG(" ++ Added LOAD SOURCE:" << ptrOp << "\n");
    taintSet.add(ptrOp);

    DOT->addRelation(load, ptrOp, "load via store");

    if (isa<Instruction>(ptrOp))
      recursivelyAddAllGepsAndLoads(cast<Instruction>(ptrOp), taintSet);
  }
}

bool FunctionProcessor::isCfgSuccessorOfPreviousStores(const StoreInst& storeInst, const TaintSet& taintSet) {
  for (TaintSet::const_iterator i = taintSet.begin(), e = taintSet.end(); i != e; ++i) {
    DEBUG_LOG(" CFG SUCC: inspecting " << **i << "\n");

    if (!isa<StoreInst>(*i))
      continue;

    const StoreInst& prevStore = *cast<StoreInst>(*i);

    if (prevStore.getOperand(1) != storeInst.getOperand(1))
      continue;

    set<const BasicBlock*> usedList;
    if (!isCfgSuccessor(storeInst.getParent(), prevStore.getParent(), usedList)) {
      return false;
    }
  }

  return true;
}

bool FunctionProcessor::isCfgSuccessor(const BasicBlock* succ, const BasicBlock* pred, set<const BasicBlock*>& usedList) {
  if (NULL == succ || NULL == pred)
    return false;

  DEBUG_LOG("CFG SUCC recursion for succ = `" << succ->getName()
          << "` and pred = `" << pred->getName() << "`\n");

  if (pred == succ)
    return true;

  for (const_pred_iterator i = pred_begin(succ), e = pred_end(succ); i != e; ++i) {
    if (usedList.count(*i))
      continue;

    DEBUG_LOG(**i << "\n");

    usedList.insert(*i);
    if (isCfgSuccessor(*i, pred, usedList))
      return true;
  }

  return false;
}

void FunctionProcessor::buildMappingFromTaintFile(const CallInst& callInst, const Function& callee, ResultSet& taintResults) {
  TaintFile* taints = TaintFile::read(callee, debug());

  if (!taints) {
    ERROR_LOG("Cannot find definition of `" << callee.getName() << "`.\n");
    _canceledInspection = true;
    _processingState = ErrorMissingDefinition;
    delete(taints);
    return;
  }

  FunctionTaintMap& mapping = taints->getMapping();
  createResultSetFromFunctionMapping(callInst, callee, mapping, taintResults);
  delete(taints);
}

/**
 * Transforms a raw mapping in the form of
 * 0 => 1, 1 => -1, ...
 * to a mapping where actual Values of the call are used
 *
 * -1 on the right hand side of => denotes the return value
 */
void FunctionProcessor::createResultSetFromFunctionMapping(const CallInst& callInst, const Function& callee, FunctionTaintMap& mapping, ResultSet& taintResults) {
  IF_PROFILING(long t = Helper::getTimestamp());

  DEBUG_LOG(" Got " << mapping.size() << " taint mappings for " << callee.getName() << "\n");
  for (FunctionTaintMap::const_iterator i = mapping.begin(), e = mapping.end(); i != e; ++i) {
    int paramPos = i->first;
    int retvalPos = i->second;

    size_t calleeArgCount = callee.getArgumentList().size();
    size_t callArgCount = callInst.getNumArgOperands();

    set<const Value*> sources;
    set<const Value*> sinks;

    DEBUG_LOG(" Converting mapping: " << paramPos << " => " << retvalPos << "\n");

    // Build set for sources
    if (paramPos == -2) {
      // VarArgs
      for (size_t i = calleeArgCount; i < callArgCount; i++) {
        sources.insert(callInst.getArgOperand(i));
      }
    } else {
      // Normal arguments
      const Value* arg = callInst.getArgOperand(paramPos);
      sources.insert(arg);
    }
    DEBUG_LOG("processed source-mappings\n");

    // Build set for sinks
    if (retvalPos == -1) {
      // Return value
      sinks.insert(&callInst);
    } else if (retvalPos == -2) {
      // Varargs
      // These can also be taint sinks, namely if pointers
      // are put as varargs -- because we cannot detect this in
      // the callee, we have to handle it here.
      for (size_t i = calleeArgCount; i < callArgCount; i++) {
        if (callInst.getArgOperand(i)->getType()->isPointerTy())
          sinks.insert(callInst.getArgOperand(i));
      }
    } else {
      // Out pointer
      const Value* returnTarget = callInst.getArgOperand(retvalPos);
      sinks.insert(returnTarget);
    }

    DEBUG_LOG("processed sink-mappings\n");

    for (set<const Value*>::iterator so_i = sources.begin(), so_e = sources.end(); so_i != so_e; so_i++) {
      const Value& source = **so_i;
      for (set<const Value*>::iterator si_i = sinks.begin(), si_e = sinks.end(); si_i != si_e; si_i++) {
        const Value& sink = **si_i;

        taintResults.insert(make_pair(&source, &sink));
      }
    }
  }

  PROFILE_LOG("createResultSetFromFunctionMapping() took " << Helper::getTimestampDelta(t) << " µs\n");
}

void FunctionProcessor::buildMappingForRecursiveCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) {
  IF_PROFILING(long t = Helper::getTimestamp());

  for (ResultSet::const_iterator i = _taints.begin(), e = _taints.end(); i != e; ++i) {
    int inPos = getArgumentPosition(func, *i->first);
    int outPos = getArgumentPosition(func, *i->second);

    Value* inVal = callInst.getArgOperand(inPos);
    const Value* outVal = outPos >= 0 ? callInst.getArgOperand(outPos) : &callInst;

    taintResults.insert(make_pair(inVal, outVal));
  }

  PROFILE_LOG("buildMappingForRecursiveCall() took " << Helper::getTimestampDelta(t) << " µs\n");
}

void FunctionProcessor::buildMappingForCircularReferenceCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) {
  IF_PROFILING(long t = Helper::getTimestamp());

  ResultSet refResult;
  FunctionProcessor refFp(PASS, func, _circularReferences, M, refResult, _stream);
  refFp._suppressPrintTaints = true;
  refFp._shouldWriteErrors = false;
  refFp.processFunction();

  _stream << "build circular ref mapping for " << func.getName() << " -- funcProc result was: " << refFp.didFinish() << "\n";
  _stream << "state was: " << refFp.getState() << "\n";

  if (!refFp.didFinish()) {
    _canceledInspection = true;

    if (refFp.getState() == ErrorMissingDefinition)
      _processingState = Deferred;
    else
      _processingState = refFp.getState();

    return;
  }

  for (ResultSet::const_iterator i = refResult.begin(), e = refResult.end(); i != e; ++i) {
    DEBUG_LOG("found mapping: " << *i->first << " => " << *i->second << "\n");
    int inPos = getArgumentPosition(func, *i->first);
    int outPos = getArgumentPosition(func, *i->second);

    if (inPos >= (int) callInst.getNumArgOperands() || inPos < -2) {
      ERROR_LOG("Argument position " << inPos << " invalid for call to `" << func.getName() << "`\n");
      _canceledInspection = true;
      _processingState = ErrorArguments;
      return;
    }

    if (outPos >= (int) callInst.getNumArgOperands() || outPos < -1) {
      ERROR_LOG("Argument position " << outPos << " invalid for call to `" << func.getName() << "`\n");
      _canceledInspection = true;
      _processingState = ErrorArguments;
      return;
    }

    Value* inVal = callInst.getArgOperand(inPos);
    const Value* outVal = outPos >= 0 ? callInst.getArgOperand(outPos) : &callInst;

    taintResults.insert(make_pair(inVal, outVal));
  }

  PROFILE_LOG("buildMappingForCircularReferenceCall() took " << Helper::getTimestampDelta(t) << " µs\n");
}

/**
 * For calls to Functions that are declared but not defined
 * in the given bc-assembly (= extern functions) we use a 
 * conservative heuristic:
 * 1) Every parameter taints the return value
 * 2) Every parameter taints all out pointers
 */
void FunctionProcessor::buildMappingForUndefinedExternalCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) {
  for (size_t i = 0; i < callInst.getNumArgOperands(); i++) {
    const Value* arg = callInst.getArgOperand(i);

    // Every argument taints the return value
    taintResults.insert(make_pair(arg, &callInst));
    DEBUG_LOG("undef-ext call: " << arg->getName() << " -> $_retval\n");

    size_t k = 0;
    for (Function::const_arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; a_i++) {
      const Argument& param = *a_i;
      Value* out = callInst.getArgOperand(k);
      k++;

      if (!out) {
        ERROR_LOG("arg #" << k << " was NULL\n");
        continue;
      }

      if (param.getType()->isPointerTy() && out != arg) {
        // Since it is a pointer it is a possible out-argument
        taintResults.insert(make_pair(arg, out));
        DEBUG_LOG("undef-ext call: " << arg->getName() << " -> " << out->getName() << "\n");
      }
    }
  }
}

void FunctionProcessor::findPossibleCallees(const Value& v, set<const Function*>& possibleCallees) {
  if (isa<LoadInst>(v)) {
    const LoadInst& callSource = cast<LoadInst>(v);
    DEBUG_LOG("Call source: " << callSource << "\n");

    const Value& delegate = *callSource.getOperand(0);

    for (Value::const_use_iterator i = delegate.use_begin(), e = delegate.use_end(); i != e; ++i) {
      if (isa<StoreInst>(*i)) {
        const StoreInst& store = *cast<StoreInst>(*i);
        findPossibleCallees(*store.getOperand(0), possibleCallees);
      }
    }
  } else if (isa<PHINode>(v)) {
    const PHINode& phi = cast<PHINode>(v);

    for (size_t j = 0; j < phi.getNumIncomingValues(); j++) {
      const Value& incoming = *phi.getIncomingValue(j);
      findPossibleCallees(incoming, possibleCallees);
    }
  } else if (isa<Argument>(v)) {
    const Argument& lambdaArg = cast<Argument>(v);
    int argPos = lambdaArg.getArgNo();

    // Find all calls to this function.
    // Add all lambda arguments as possible callees.
    for (Value::const_use_iterator i = F.use_begin(), e = F.use_end(); i != e; ++i) {
      if (isa<CallInst>(*i)) {
        const CallInst& call = cast<CallInst>(**i);
        DEBUG_LOG("Use of function: " << call << "\n");

        const Value& lambda = *call.getArgOperand(argPos);
        findPossibleCallees(lambda, possibleCallees);
      } else {
        ERROR_LOG("Use of Function not a CallInst\n");
        _canceledInspection = true;
        _processingState = Error;
        return;
      }
    }
  } else if (isa<Function>(v)) {
    DEBUG_LOG("Found possible function: " << v.getName() << "\n");
    possibleCallees.insert(cast<Function>(&v));
  } else {
    ERROR_LOG("Cannot de-reference function pointer: " << v << "\n");
    _canceledInspection = true;
    _processingState = Error;
  }
}

void FunctionProcessor::handleCallInstruction(const CallInst& callInst, TaintSet& taintSet) {
  DEBUG_LOG(" Handle CALL instruction:\n");
  const Function* callee = callInst.getCalledFunction();
  DEBUG_LOG(" Callee:" << *callInst.getCalledValue() << "\n");
  
  // Skip inline ASM for now
  if (callInst.isInlineAsm()) {
    DEBUG_LOG(" Ignoring inline ASM. \n");
    return;
  }

  if (callee != NULL) {
    // We are calling to a 'normal' function 
    handleFunctionCall(callInst, *callee, taintSet);
  } else {
    // We are calling to a function pointer.
    // We search for all possible aliases and
    // execute a call to each possible realisation.
    // This effectively builds the taint-union for all
    // possible realisations.
    set<const Function*> possibleCallees;
    findPossibleCallees(*callInst.getCalledValue(), possibleCallees);
    STOP_ON_CANCEL;

    for (set<const Function*>::iterator c_i = possibleCallees.begin(), c_e = possibleCallees.end(); c_i != c_e; ++c_i) {
      const Function& callee = **c_i;
      DEBUG_LOG("Possible function: " << callee.getName() << "\n");

      // If the indirect function itself is tainted (eg. function pointer)
      // add the return value to taints.       
      if (taintSet.contains(callee)) {
        taintSet.add(callInst);
        DOT->addRelation(*callInst.getCalledValue(), callee, "function indirection");
      }

      // If the function pointer is tainted, add the callees.
      if (taintSet.contains(*callInst.getCalledValue())) {
        taintSet.add(callee);
        DOT->addRelation(*callInst.getCalledValue(), callee, "function indirection");
      }

      // Handling all indirect calls produces a union of the taints
      // transferred for all possibly called functions.
      handleFunctionCall(callInst, callee, taintSet);
    }
  }
}

void FunctionProcessor::handleFunctionCall(const CallInst& callInst, const Function& callee, TaintSet& taintSet) {
  DEBUG_LOG(" * Calling function `" << callee.getName() << "`\n");

  ResultSet taintResults;
  long t;

  if (&callee == &F) {
    // build intermediate taint sets
    t = Helper::getTimestamp();
    buildResultSet(false);
    PROFILE_LOG(" buildResultSet() took " << Helper::getTimestampDelta(t) << "\n");

    buildMappingForRecursiveCall(callInst, callee, taintResults);

  } else if (TaintFile::exists(callee) && !Helper::circleListContains(_circularReferences[&F], callee)) {
    t = Helper::getTimestamp();
    buildMappingFromTaintFile(callInst, callee, taintResults);
    PROFILE_LOG(" buildMappingFromTaintFile() took " << Helper::getTimestampDelta(t) << "\n");
  } else if (callee.isIntrinsic()) {
    DEBUG_LOG("handle intrinsic call: " << callee.getName() << "\n");

    FunctionTaintMap mapping;
    if (IntrinsicHelper::getMapping(callee, mapping)) {
      createResultSetFromFunctionMapping(callInst, callee, mapping, taintResults);
    } else {
      ERROR_LOG("No definition of intrinsic `" << callee.getName() << "`.\n");
      _canceledInspection = true;
      _processingState = ErrorMissingIntrinsic;
    }

  } else if (Helper::circleListContains(_circularReferences[&F], callee)) {
    DEBUG_LOG("calling with circular reference: " << F.getName() << " (caller) <--> (callee) " << callee.getName() << "\n");

    if (TaintFile::exists(callee)) {
      buildMappingFromTaintFile(callInst, callee, taintResults);
    } else {
      // Write out currently available taint mapping of this function
      // because the dependent function need this information.
      buildResultSet(false);
      TaintFile::writeResult(F, _taints);

      buildMappingForCircularReferenceCall(callInst, callee, taintResults);

      TaintFile::remove(F);
    }
  } else if (callee.size() == 0 || callInst.isInlineAsm()) {
    // External functions
    DEBUG_LOG("calling to undefined external. using heuristic.\n");
    buildMappingForUndefinedExternalCall(callInst, callee, taintResults);
  } else {
    ERROR_LOG("Could not evaluate function call. " << callee.getName() << "\n");
    _canceledInspection = true;
    _processingState = ErrorMissingDefinition;
  }

  bool needToAddGraphNodeForFunction = false;
  for (ResultSet::const_iterator i = taintResults.begin(), e = taintResults.end(); i != e; ++i) {
    const Value& in = *i->first;
    const Value& out = *i->second;

    if (taintSet.contains(in)) {
      // Add graph arrows and function-node only if taints
      // were found. Otherwise the function-node would be
      // orphaned in the graph.
      needToAddGraphNodeForFunction = true;
      DEBUG_LOG("in is: " << in << "\n");
      stringstream reas("");
      reas << "in, arg#" << getArgumentPosition(callInst, in);
      DOT->addRelation(in, callee, reas.str());

      if (taintSet.contains(callee) && out.getType()->isPointerTy()) {
        DOT->addRelation(callee, out, "function-indirection");
        taintSet.add(out);
      }

      taintSet.add(out);
      if (&out == &callInst) {
        DOT->addRelation(callee, callInst, "ret");
      } else {
        stringstream outReas("");
        outReas << "out, arg#" << getArgumentPosition(callInst, out);
        DOT->addRelation(callee, out, outReas.str());
      }

      // Value is a pointer, so the previous load is also tainted.
      if (isa<LoadInst>(out)) {
        Value* op = (cast<LoadInst>(out)).getOperand(0);
        taintSet.add(*op);
        DEBUG_LOG(" ++ Added previous load: " << out << "\n");
        DOT->addRelation(*op, out, "load");
      }
    }
  }


  if (needToAddGraphNodeForFunction)
    DOT->addCallNode(callee);
}

/**
 * Search the argument position for the given Value in
 * the given CallInst.
 *
 * @return the position of the argument in this call, -3 if not found
 */
int FunctionProcessor::getArgumentPosition(const CallInst& c, const Value& v) {
  for (size_t i = 0; i < c.getNumArgOperands(); ++i) {
    if (c.getArgOperand(i) == &v)
      return i;
  }

  return -3;
}

/**
 * Search the parameter position for the given Value in
 * the given Function.
 *
 * @return the position of the parameter in the corresponding Function, -3 if not found
 */
int FunctionProcessor::getArgumentPosition(const Function& f, const Value& v) {
  if (isa<ReturnInst>(v))
    return -1;

  if (isa<Argument>(v))
    return (cast<Argument>(v)).getArgNo();

  return -3;
}


/**
 * SWITCH is handled the following way:
 *
 * If the condition is tainted, each case (or default) is tainted
 * due to nesting.
 */
void FunctionProcessor::handleSwitchInstruction(const SwitchInst& inst, TaintSet& taintSet) {
  const Value* condition = inst.getCondition();
  
  if (!taintSet.contains(*condition))
    return;

  DOT->addRelation(*condition, inst, "switch");

  DEBUG_LOG(" Handle SWITCH instruction:\n");
  for (size_t i = 0; i < inst.getNumSuccessors(); ++i) {
    // Mark all case-blocks as tainted.
    const BasicBlock& caseBlock = *inst.getSuccessor(i);
    DOT->addBlockNode(caseBlock);
    DOT->addRelation(inst, caseBlock, "case");
    taintSet.add(caseBlock);
    DEBUG_LOG(" + Added Block due to tainted SWITCH condition: " << caseBlock << "\n");
  }
}

void FunctionProcessor::handlePhiNode(const PHINode& inst, TaintSet& taintSet) {
  for (size_t j = 0; j < inst.getNumIncomingValues(); ++j) {
    BasicBlock& incomingBlock = *inst.getIncomingBlock(j);
    Value& incomingValue = *inst.getIncomingValue(j);

    // We need to handle block-tainting here, because
    // with PHI nodes, the effective assignment is no
    // longer in the previous block, but in the PHI.
    // If you assign constants in the block and the block
    // is tainted by an if, we would not see this taint,
    // because our logic would say: "is <const 7> tainted. "no".
    // Without phi we would have an explicit (block tainted) assignment
    // or store in the block.
    if (taintSet.contains(incomingBlock)) {
      DEBUG_LOG(" + Added PHI from block" << incomingBlock << "\n");
      taintSet.add(inst);
      DOT->addRelation(incomingBlock, inst, "block-taint");
    } else if (taintSet.contains(incomingValue)) {
      DEBUG_LOG(" + Added PHI from value" << incomingValue << "\n");
      taintSet.add(inst);
      DOT->addRelation(incomingValue, inst, "phi-value");
    }
  }
}

void FunctionProcessor::handleBranchInstruction(const BranchInst& inst, TaintSet& taintSet) {
  DEBUG_LOG(" Handle BRANCH instruction: " << inst << "\n");
  
  if (inst.isConditional()) {
    const Value& cmp = *inst.getCondition();
    DEBUG_LOG(" = Compare instruction is: " << cmp << "\n");

    bool isConditionTainted = taintSet.contains(cmp);
    if (isConditionTainted) {
      DOT->addRelation(cmp, inst, "condition");
     
      const BasicBlock& brTrue = *inst.getSuccessor(0);
      const BasicBlock& brFalse = *inst.getSuccessor(1);

      const BasicBlock& join = *const_cast<const BasicBlock*>(PDT.findNearestCommonDominator(const_cast<BasicBlock*>(&brFalse), const_cast<BasicBlock*>(&brTrue)));

      DEBUG_LOG("   Nearest Common Post-Dominator for tr/fa: " << join << "\n");
      
      // true branch is always tainted
      taintSet.add(brTrue);
      DOT->addBlockNode(brTrue);
      DOT->addRelation(inst, brTrue, "br-true");
      DEBUG_LOG(" + Added TRUE branch to taint set:\n");
      DEBUG_LOG(brTrue.getName() << "\n");

      followTransientBranchPaths(brTrue, join, taintSet);

      // false branch is only tainted if successor 
      // is not the same as jump target after true branch
      if (&join != &brFalse) {
        taintSet.add(brFalse);
        DOT->addBlockNode(brFalse);
        DOT->addRelation(inst, brFalse, "br-false");
        DEBUG_LOG(" + Added FALSE branch to taint set:\n");
        DEBUG_LOG(brFalse.getName() << "\n");

        followTransientBranchPaths(brFalse, join, taintSet);
     }
    }
  } else {
    if (taintSet.hasChanged()) {
      const BasicBlock* target = inst.getSuccessor(0);
      enqueueBlockToWorklist(target);
      DEBUG_LOG("Added block " << target->getName() << " for reinspection due to UNCONDITIONAL JUMP\n");
    }
  }
}

void FunctionProcessor::enqueueBlockToWorklist(const BasicBlock* block) {
  _workList.push_front(block);
}

void FunctionProcessor::followTransientBranchPaths(const BasicBlock& br, const BasicBlock& join, TaintSet& taintSet) {
  const TerminatorInst& brTerminator = *br.getTerminator();

  if (brTerminator.getNumSuccessors() != 1)
    return;

  const BasicBlock& brSuccessor = *brTerminator.getSuccessor(0);

  if (PDT.dominates(&join, &brSuccessor) && 
      &brSuccessor != &join) {

    taintSet.add(brSuccessor);
    DOT->addBlockNode(brSuccessor);
    DOT->addRelation(brTerminator, brSuccessor, "br");
    DEBUG_LOG(" ++ Added TRANSIENT branch:\n");
    DEBUG_LOG(brSuccessor << "\n");

    followTransientBranchPaths(brSuccessor, join, taintSet);
  }
}

/**
 * An arbitrary Instruction is handled the following way:
 *
 * If one of the operands is tainted, the taitn is transfered to
 * the assignment target.
 */
void FunctionProcessor::handleInstruction(const Instruction& inst, TaintSet& taintSet) {

  for (size_t o_i = 0; o_i < inst.getNumOperands(); o_i++) {
     const Value& operand = *inst.getOperand(o_i);
     if (taintSet.contains(operand)) {
       taintSet.add(inst);
       DOT->addRelation(operand, inst, string(Instruction::getOpcodeName(inst.getOpcode())));

       DEBUG_LOG(" + Added " << operand << " --> " << inst << "\n");
    }
  }
}

bool FunctionProcessor::isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet) {
  bool result = false;

  for (TaintSet::const_iterator s_i = taintSet.begin(), s_e = taintSet.end(); s_i != s_e; ++s_i) {
    if (*s_i == NULL)
      continue;

    assert (*s_i != NULL && "BB must not be NULL");
    if (! isa<BasicBlock>(*s_i))
      continue;

    const BasicBlock& taintedBlock = cast<BasicBlock>(**s_i);

    if (DT.dominates(&taintedBlock, &currentBlock)) {
      DEBUG_LOG(" ! Dirty block `" << taintedBlock.getName() << "` dominates `" << currentBlock.getName() << "`\n");

      if (&taintedBlock != &currentBlock) {
        taintSet.add(currentBlock);
        DOT->addBlockNode(currentBlock);
        DOT->addRelation(taintedBlock, currentBlock, "block-taint");
      }

      result = true;
    }
  }

  return result;
}

void FunctionProcessor::handleBlockTainting(const Instruction& inst, const BasicBlock& currentBlock, TaintSet& taintSet) {
  DEBUG_LOG(" Handle BLOCK-tainting for `" << inst << "`\n");

  // Loads should not be tained by parenting block
  // because otherwise a block would taint a load of
  // a global variable what makes no sense -- it would
  // introduce a taint that does not exist.
  if (isa<LoadInst>(inst)) {
    DEBUG_LOG(" Ignoring LOAD\n");
    return;
  }

  if (isa<GetElementPtrInst>(inst)) {
    DEBUG_LOG(" Ignoring GEP\n");
    return;
  }

  taintSet.add(inst);
  if (isa<StoreInst>(inst))
    DOT->addRelation(currentBlock, *inst.getOperand(0), "block-taint");
  else
    DOT->addRelation(currentBlock, inst, "block-taint");

  DEBUG_LOG(" + Instruction tainted by dirty block: " << inst << "\n");
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

  DEBUG_LOG(" -- Inspecting argument or global `" << arg.getName() << "`\n");

  if ((arg.getType()->isPointerTy() || isa<GlobalVariable>(arg))) {
    TaintSet returnSet;
    returnSet.add(arg);

    findAllStoresAndLoadsForOutArgumentAndAddToSet(arg, returnSet);

    _returnStatements.insert(make_pair(&arg, returnSet));
    DOT->addInOutNode(arg);
    isInOutNode = true;

    DEBUG_LOG("added arg `" << arg.getName() << "` to out-list\n");
  }

  TaintSet taintSet;
  taintSet.add(arg);
  if (!isInOutNode)
    DOT->addInNode(arg);

  _arguments.insert(make_pair(&arg, taintSet));
  DEBUG_LOG("added arg `" << arg.getName() << "` to arg-list\n");
}

void FunctionProcessor::findAllStoresAndLoadsForOutArgumentAndAddToSet(const Value& arg, ReturnSet& returnSet) {
    const Value* newArg = &arg;

    for (User::const_use_iterator u_i = arg.use_begin(), u_e = arg.use_end(); u_i != u_e; ++u_i) {
      if (isa<StoreInst>(*u_i)) {
        returnSet.add(**u_i);
        DEBUG_LOG(" Added ARG STORE: " << **u_i << "\n");
        newArg = u_i->getOperand(1);
      }
    }

    ReturnSet alreadyProcessed;
    recursivelyFindAliases(*newArg, returnSet, alreadyProcessed);
}

void FunctionProcessor::recursivelyFindAliases(const Value& arg, ReturnSet& returnSet, ReturnSet& alreadyProcessed) {

  DEBUG_LOG("Recursively find: " << arg << "\n");

  if (alreadyProcessed.contains(arg))
    return;

  alreadyProcessed.add(arg);

  for (User::const_use_iterator i = arg.use_begin(), e = arg.use_end(); i != e; ++i) {
    if (!isa<Instruction>(**i)) {
      // Necessary, as constant string literals also come 
      // as GlobalVariables, but do not point to an instruction.
      // They point to an operand of a GEP.
      DEBUG_LOG("Skip: " << **i << "\n");
      continue;
    }

    const Instruction& I = cast<Instruction>(**i);
    
    DEBUG_LOG("Inspecting: " << I << "\n");

    if (isa<LoadInst>(I) && I.getOperand(0) == &arg) {
      const Value& load = cast<LoadInst>(I);
      returnSet.add(load);
      DOT->addRelation(arg, load, "load");
      DEBUG_LOG(" + Found LOAD for `" << arg.getName() << "` @ " << load << "\n");
      
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
          DEBUG_LOG(" + Added instruction CONSTANT RETURN VALUE `" << *retval << "`\n");
        } else {
          taintSet.add(*retval);
          DEBUG_LOG(" + Added NON-CONST RETURN VALUE `" << retval << "`\n");
        }

        _returnStatements.insert(make_pair(&r, taintSet));
        DOT->addOutNode(r);
        DEBUG_LOG("Found ret-stmt: " << r << "\n");
      }
    }
  }
}

void FunctionProcessor::printInstructions() {
  DEBUG_LOG("Instructions: \n");

  for (const_inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    DEBUG_LOG(i->getParent() << " | " << &*i << " | " << (*i) << "\n");
  }
}
