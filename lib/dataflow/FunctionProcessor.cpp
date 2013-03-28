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
#include <map>
#include <set>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include "FunctionProcessor.h"
#include "TaintFile.h" 

#define STOP_ON_CANCEL if (_canceledInspection) return
#define DEBUG_LOG(x) DEBUG(debug() << x)
#define PROFILE_LOG(x) DEBUG(debug() << x)

using namespace llvm;
using namespace std;

bool FunctionProcessor::didFinish() {
  return !_canceledInspection;
}

void FunctionProcessor::processFunction() {
  printInstructions();

  findReturnStatements();
  findArguments();

  int resultIteration = 0;

  do {
    _resultSetChanged = false;

    TaintMap::iterator arg_i = _arguments.begin();
    TaintMap::iterator arg_e = _arguments.end();
      
    for(; arg_i != arg_e; ++arg_i) {
      const Value& arg = *arg_i->first;
      TaintSet& taintSet = arg_i->second;

      _taintSetChanged = false;

      buildTaintSetFor(arg, taintSet);
      STOP_ON_CANCEL;
      resultIteration++;
    }

    buildResultSet();
  } while (resultIteration < 100 && _resultSetChanged);

  if (!_suppressPrintTaints)
    printTaints();
}

void FunctionProcessor::buildResultSet() {

  TaintMap::const_iterator arg_i = _arguments.begin();
  TaintMap::const_iterator arg_e = _arguments.end();
      
  for(; arg_i != arg_e; ++arg_i) {
    const Value& arg = *arg_i->first;
    const TaintSet& taintSet = arg_i->second;

    intersectSets(arg, taintSet);
  }
}

void FunctionProcessor::intersectSets(const Value& arg, const TaintSet argTaintSet) {
  TaintMap::const_iterator ret_i = _returnStatements.begin();
  TaintMap::const_iterator ret_e = _returnStatements.end();

  for (; ret_i != ret_e; ++ret_i) {
    const Value& retval = *ret_i->first;
    const TaintSet retTaintSet = ret_i->second;

    if (&retval == &arg) {
      DEBUG_LOG("Skipping detected self-taint\n");
      continue;
    }

    long t = Helper::getTimestamp();
    DEBUG_LOG("Ret-set for `" << retval << "`:\n");
    printSet(retTaintSet);
    PROFILE_LOG("printSet() took " << Helper::getTimestampDelta(t) << " µs\n");

    t = Helper::getTimestamp();
    TaintSet intersect;
    Helper::intersectSets(argTaintSet, retTaintSet, intersect);
    PROFILE_LOG("intersect() took " << Helper::getTimestampDelta(t) << " µs\n");

    if (intersect.size()) {
      addTaint(arg, retval);

      DEBUG_LOG("Values that lead to taint " << Helper::getValueNameOrDefault(arg) << " -> "
              << Helper::getValueNameOrDefault(retval) << ":\n");
      printSet(intersect);
    }
  }
}

inline void FunctionProcessor::addTaintToSet(TaintSet& taintSet, const Value& v) {
  long t = Helper::getTimestamp();
  if (taintSet.insert(&v))
    _taintSetChanged = true;

  PROFILE_LOG(" addTaintToSet took: " << Helper::getTimestampDelta(t) << "µs\n");
}

void FunctionProcessor::buildTaintSetFor(const Value& arg, TaintSet& taintSet) {
  DEBUG_LOG(" *** Creating taint set for argument `" << arg.getName() << "`\n");

  // Arg trivially taints itself.
  addTaintToSet(taintSet, arg);

  _blockList.clear();
  _workList.clear();

  for (Function::const_iterator b_i = F.begin(), b_e = F.end(); b_i != b_e; ++b_i) {
    const BasicBlock& block = cast<BasicBlock>(*b_i);
    TaintSet blockTaintSet;
    addTaintToSet(blockTaintSet, arg);
    _blockList.insert(make_pair(&block, blockTaintSet));
    _workList.push_back(&block);
  }

  do {
    STOP_ON_CANCEL;

    _taintSetChanged = false;

    while (!_workList.empty()) {
      const BasicBlock& block = *_workList.front();
      _workList.pop_front();

      DEBUG_LOG(" ----- PROCESS BLOCK " << block.getName() << " -----\n");
      applyMeet(block);

      processBasicBlock(block, _blockList[&block]);
    }

  } while(_taintSetChanged);

  for (map<const BasicBlock*, TaintSet>::const_iterator j_i = _blockList.begin(), j_e = _blockList.end(); j_i != j_e; ++j_i) {
    TaintSet& set = _blockList[j_i->first];

    for (TaintSet::const_iterator t_i = set.begin(), t_e = set.end(); t_i != t_e; ++t_i) {
      taintSet.insert(*t_i);
    }
  }

  DEBUG_LOG("Taint set for arg `" << arg.getName() << "`:\n");
  printSet(taintSet);
}

void FunctionProcessor::applyMeet(const BasicBlock& block) {
  TaintSet& blockSet = _blockList[&block];
  DEBUG_LOG("Applying meet for: " << block.getName() << "\n");
  
  for (const_pred_iterator i = pred_begin(&block), e = pred_end(&block); i != e; ++i) {
    const BasicBlock& pred = **i;
    DEBUG_LOG("Meeting block: " << pred.getName() << "\n");

    TaintSet& predSet = _blockList[&pred];
    for (TaintSet::const_iterator s_i = predSet.begin(), s_e = predSet.end(); s_i != s_e; ++s_i) {
      blockSet.insert(*s_i);
      DEBUG_LOG("Inserting " << **s_i << "\n");
    }
  }

  DEBUG_LOG("End meet\n");
}

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

    long t = Helper::getTimestamp();

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

  if (Helper::setContains(taintSet, op)) {
    addTaintToSet(taintSet, inst);
    DOT.addRelation(op, inst, "indexer");
    DEBUG_LOG(" + Added GEP taint: `" << inst << "`\n");
  }
  
  for (size_t i = 0; i < inst.getNumIndices(); i++) {
    const Value& idx = *inst.getOperand(i + 1);

    if (Helper::setContains(taintSet, idx)) {
      addTaintToSet(taintSet, inst);
      stringstream reason("");
      reason << "index #" << i;
      DOT.addRelation(idx, inst, reason.str());
      DEBUG_LOG(" ++ Added GEP INDEX: `" << idx << "`\n");
    }
  }
}

void FunctionProcessor::handleStoreInstruction(const StoreInst& storeInst, TaintSet& taintSet) {
  const Value& source = *storeInst.getOperand(0);
  const Value& target = *storeInst.getOperand(1);

  DEBUG_LOG(" Handle STORE instruction " << storeInst << "\n");
  if (Helper::setContains(taintSet, source) || Helper::setContains(taintSet, storeInst)) {
    addTaintToSet(taintSet, target);
    addTaintToSet(taintSet, storeInst);

    DOT.addRelation(source, target, "store");
    DEBUG_LOG(" + Added STORE taint: " << source << " --> " << target << "\n");
    if (isa<GetElementPtrInst>(target)) {
      const GetElementPtrInst& gep = cast<GetElementPtrInst>(target);
      recursivelyAddAllGeps(gep, taintSet);
    }
  } else if (Helper::setContains(taintSet, target) && isCfgSuccessorOfPreviousStores(storeInst, taintSet)) {
    // Only do removal if value is really in set
    taintSet.erase(&target);
    DEBUG_LOG(" - Removed STORE taint due to non-tainted overwrite: " << source << " --> " << target << "\n");

    if (isa<LoadInst>(target)) {
      const Value* load = (cast<LoadInst>(target)).getPointerOperand();
      taintSet.erase(load);
      DEBUG_LOG(" - Also removed transitive LOAD." << *load << "\n");
    }

    DOT.addRelation(source, target, "non-taint overwrite");
  }
}

void FunctionProcessor::recursivelyAddAllGeps(const GetElementPtrInst& gep, TaintSet& taintSet) {
  const Value& ptrOp = *gep.getPointerOperand();
  DEBUG_LOG(" ++ Added GEP SOURCE:" << ptrOp << "\n");
  addTaintToSet(taintSet, ptrOp);

  DOT.addRelation(gep, ptrOp, "gep via store");

  if (isa<LoadInst>(ptrOp)) {
    const LoadInst& load = cast<LoadInst>(ptrOp);
    const Value& loadOperand = *load.getOperand(0);
    if (isa<GetElementPtrInst>(loadOperand))
      recursivelyAddAllGeps(cast<GetElementPtrInst>(loadOperand), taintSet);
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

  DEBUG_LOG("CFG SUCC recursion for succ = `" << *succ
          << "` and pred = `" << *pred << "`\n");

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

void FunctionProcessor::readTaintsFromFile(const CallInst& callInst, const Function& func, ResultSet& taintResults) {
  TaintFile* taints = TaintFile::read(func, debug());

  if (!taints) {
    DEBUG_LOG(" -- Cannot get information about `" << func.getName() << "` -- cancel.\n");
    _canceledInspection = true;
    return;
  }

  FunctionTaintMap& mapping = taints->getMapping();

  for (FunctionTaintMap::const_iterator i = mapping.begin(), e = mapping.end(); i != e; ++i) {
    int paramPos = i->first;
    int retvalPos = i->second;

    DEBUG_LOG(" Use mapping: " << paramPos << " => " << retvalPos << "\n");
    const Value* arg = callInst.getArgOperand(paramPos);

    if (retvalPos == -1) {
      taintResults.insert(make_pair(arg, &callInst));
    }
    else {
      const Value* returnTarget = callInst.getArgOperand(retvalPos);
      taintResults.insert(make_pair(arg, returnTarget));
    }
  }

  delete(taints);
}

void FunctionProcessor::buildMappingForRecursiveCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) {
  for (ResultSet::const_iterator i = _taints.begin(), e = _taints.end(); i != e; ++i) {
    int inPos = getArgumentPosition(func, *i->first);
    int outPos = getArgumentPosition(func, *i->second);

    Value* inVal = callInst.getArgOperand(inPos);
    const Value* outVal = outPos >= 0 ? callInst.getArgOperand(outPos) : &callInst;

    taintResults.insert(make_pair(inVal, outVal));
  }
}

void FunctionProcessor::buildMappingForCircularReferenceCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) {
  ResultSet refResult;
  FunctionProcessor refFp(PASS, func, _circularReferences, M, refResult, _stream);
  refFp._suppressPrintTaints = true;
  refFp.processFunction();

  for (ResultSet::const_iterator i = refResult.begin(), e = refResult.end(); i != e; ++i) {
    int inPos = getArgumentPosition(func, *i->first);
    int outPos = getArgumentPosition(func, *i->second);

    Value* inVal = callInst.getArgOperand(inPos);
    const Value* outVal = outPos >= 0 ? callInst.getArgOperand(outPos) : &callInst;

    taintResults.insert(make_pair(inVal, outVal));
  }
}

void FunctionProcessor::handleCallInstruction(const CallInst& callInst, TaintSet& taintSet) {
  DEBUG_LOG(" Handle CALL instruction:\n");
  const Function* callee = callInst.getCalledFunction();

  if (callee != NULL) {
    DEBUG_LOG(" * Calling function `" << callee->getName() << "`\n");

    ResultSet taintResults;
    long t;

    DOT.addCallNode(*callee);

    if (callee == &F) {
      // build intermediate taint sets
      t = Helper::getTimestamp();
      buildResultSet();
      PROFILE_LOG(" buildResultSet() took " << Helper::getTimestampDelta(t) << "\n");

      buildMappingForRecursiveCall(callInst, *callee, taintResults);

    } else if (_circularReferences.count(make_pair(&F, callee))) {
      DEBUG_LOG("calling with circular reference: " << F.getName() << " (caller) <--> (callee) " << callee->getName() << "\n");

      if (TaintFile::exists(*callee)) {
        readTaintsFromFile(callInst, *callee, taintResults);
      } else {
        buildResultSet();
        TaintFile::writeResult(F, _taints);

        buildMappingForCircularReferenceCall(callInst, *callee, taintResults);

        TaintFile::remove(F);
      }
    } else {
      t = Helper::getTimestamp();
      readTaintsFromFile(callInst, *callee, taintResults);
      PROFILE_LOG(" readTaintsFromFile() took " << Helper::getTimestampDelta(t) << "\n");
    }

    for (ResultSet::const_iterator i = taintResults.begin(), e = taintResults.end(); i != e; ++i) {
      const Value& in = *i->first;
      const Value& out = *i->second;

      stringstream reas("");
      reas << "in, arg#" << getArgumentPosition(callInst, in);
      DOT.addRelation(in, *callee, reas.str());

      if (Helper::setContains(taintSet, in)) {
        addTaintToSet(taintSet, out);
        if (&out == &callInst) {
          DOT.addRelation(*callee, callInst, "ret");
        } else {
          stringstream outReas("");
          outReas << "out, arg#" << getArgumentPosition(callInst, out);
          DOT.addRelation(*callee, out, outReas.str());
        }

        // Value is a pointer, so the previous load is also tainted.
        if (isa<LoadInst>(out)) {
          Value* op = (cast<LoadInst>(out)).getOperand(0);
          addTaintToSet(taintSet, *op);
          DEBUG_LOG(" ++ Added previous load: " << out << "\n");
          DOT.addRelation(*op, out, "load");
        }
      }
    }

  } else {
    DEBUG_LOG(" ! Cannot get information about callee `" << *callInst.getCalledValue() << "`\n");
  }
}

int FunctionProcessor::getArgumentPosition(const CallInst& c, const Value& v) {
  for (size_t i = 0; i < c.getNumArgOperands(); ++i) {
    if (c.getArgOperand(0) == &v)
      return i;
  }

  return -1;
}

int FunctionProcessor::getArgumentPosition(const Function& f, const Value& v) {

  int j = 0;
  for (Function::const_arg_iterator i = f.arg_begin(), e = f.arg_end(); i != e; ++i, ++j) {
    if (&*i == &v)
      return j;
  }

  return -1;
}

void FunctionProcessor::handleSwitchInstruction(const SwitchInst& inst, TaintSet& taintSet) {
  const Value* condition = inst.getCondition();
  
  if (!Helper::setContains(taintSet, *condition))
    return;

  DOT.addRelation(*condition, inst, "switch");

  DEBUG_LOG(" Handle SWITCH instruction:\n");
  for (size_t i = 0; i < inst.getNumSuccessors(); ++i) {
    // Mark all case-blocks as tainted.
    const BasicBlock& caseBlock = *inst.getSuccessor(i);
    DOT.addBlockNode(caseBlock);
    DOT.addRelation(inst, caseBlock, "case");
    addTaintToSet(taintSet, caseBlock);
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
    if (Helper::setContains(taintSet, incomingBlock)) {
      DEBUG_LOG(" + Added PHI from block" << incomingBlock << "\n");
      addTaintToSet(taintSet, inst);
      DOT.addRelation(incomingBlock, inst, "phi-block");
    } else if (Helper::setContains(taintSet, incomingValue)) {
      DEBUG_LOG(" + Added PHI from value" << incomingValue << "\n");
      addTaintToSet(taintSet, inst);
      DOT.addRelation(incomingValue, inst, "phi-value");
    }
  }
}

void FunctionProcessor::handleBranchInstruction(const BranchInst& inst, TaintSet& taintSet) {
  DEBUG_LOG(" Handle BRANCH instruction: " << inst << "\n");
  
  if (inst.isConditional()) {
    const Instruction& cmp = cast<Instruction>(*inst.getCondition());
    DEBUG_LOG(" = Compare instruction is: " << cmp << "\n");

    bool isConditionTainted = Helper::setContains(taintSet, cmp);
    if (isConditionTainted) {
      DOT.addRelation(cmp, inst, "condition");
     
      const BasicBlock& brTrue = *inst.getSuccessor(0);
      const BasicBlock& brFalse = *inst.getSuccessor(1);

      const BasicBlock& join = *const_cast<const BasicBlock*>(PDT.findNearestCommonDominator(const_cast<BasicBlock*>(&brFalse), const_cast<BasicBlock*>(&brTrue)));

      DEBUG_LOG("   Nearest Common Post-Dominator for tr/fa: " << join << "\n");
      
      // true branch is always tainted
      addTaintToSet(taintSet, brTrue);
      DOT.addBlockNode(brTrue);
      DOT.addRelation(inst, brTrue, "br-true");
      DEBUG_LOG(" + Added TRUE branch to taint set:\n");
      DEBUG_LOG(brTrue.getName() << "\n");

      followTransientBranchPaths(brTrue, join, taintSet);

      // false branch is only tainted if successor 
      // is not the same as jump target after true branch
      if (&join != &brFalse) {
        addTaintToSet(taintSet, brFalse);
        DOT.addBlockNode(brFalse);
        DOT.addRelation(inst, brFalse, "br-false");
        DEBUG_LOG(" + Added FALSE branch to taint set:\n");
        DEBUG_LOG(brFalse.getName() << "\n");

        followTransientBranchPaths(brFalse, join, taintSet);
     }
    }
  } else {
    if (_taintSetChanged) {
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

    addTaintToSet(taintSet, brSuccessor);
    DOT.addBlockNode(brSuccessor);
    DOT.addRelation(brTerminator, brSuccessor, "br");
    DEBUG_LOG(" ++ Added TRANSIENT branch:\n");
    DEBUG_LOG(brSuccessor << "\n");

    followTransientBranchPaths(brSuccessor, join, taintSet);
  }
}

void FunctionProcessor::handleInstruction(const Instruction& inst, TaintSet& taintSet) {

  for (size_t o_i = 0; o_i < inst.getNumOperands(); o_i++) {
     const Value& operand = *inst.getOperand(o_i);
     if (Helper::setContains(taintSet, operand)) {
       addTaintToSet(taintSet, inst);
       DOT.addRelation(operand, inst, string(Instruction::getOpcodeName(inst.getOpcode())));

       DEBUG_LOG(" + Added " << operand << " --> " << inst << "\n");
    }
  }
}

bool FunctionProcessor::isBlockTaintedByOtherBlock(const BasicBlock& currentBlock, TaintSet& taintSet) {
  bool result = false;
  for (TaintSet::const_iterator s_i = taintSet.begin(), s_e = taintSet.end(); s_i != s_e; ++s_i) {
    if (! isa<BasicBlock>(*s_i))
      continue;

    const BasicBlock& taintedBlock = cast<BasicBlock>(**s_i);

    if (DT.dominates(&taintedBlock, &currentBlock)) {
      DEBUG_LOG(" ! Dirty block `" << taintedBlock.getName() << "` dominates `" << currentBlock.getName() << "`\n");

      if (&taintedBlock != &currentBlock) {
        addTaintToSet(taintSet, currentBlock);
        DOT.addBlockNode(currentBlock);
        DOT.addRelation(taintedBlock, currentBlock, "block-taint");
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

  addTaintToSet(taintSet, inst);
  if (isa<StoreInst>(inst))
    DOT.addRelation(currentBlock, *inst.getOperand(0), "block-taint");
  else
    DOT.addRelation(currentBlock, inst, "block-taint");

  DEBUG_LOG(" + Instruction tainted by dirty block: " << inst << "\n");
}

void FunctionProcessor::findArguments() {
  for (Function::const_arg_iterator a_i = F.arg_begin(), a_e = F.arg_end(); a_i != a_e; ++a_i) {
    handleFoundArgument(*a_i);
  }

  for (Module::const_global_iterator g_i = M.global_begin(), g_e = M.global_end(); g_i != g_e; ++g_i) {
    handleFoundArgument(*g_i);
  }
}

void FunctionProcessor::handleFoundArgument(const Value& arg) {
  bool isInOutNode = false;

  DEBUG_LOG(" -- Inspecting argument or global `" << arg.getName() << "`\n");

  if (arg.getType()->isPointerTy() || isa<GlobalVariable>(arg)) {
    TaintSet returnSet;
    returnSet.insert(&arg);

    findAllStoresAndLoadsForOutArgumentAndAddToSet(arg, returnSet);

    _returnStatements.insert(make_pair(&arg, returnSet));
    DOT.addInOutNode(arg);
    isInOutNode = true;

    DEBUG_LOG("added arg `" << arg.getName() << "` to out-list\n");
  }

  TaintSet taintSet;
  addTaintToSet(taintSet, arg);
  if (!isInOutNode)
    DOT.addInNode(arg);

  _arguments.insert(make_pair(&arg, taintSet));
  DEBUG_LOG("added arg `" << arg.getName() << "` to arg-list\n");
}

void FunctionProcessor::findAllStoresAndLoadsForOutArgumentAndAddToSet(const Value& arg, ReturnSet& returnSet) {
    const Value* newArg = &arg;

    for (User::const_use_iterator u_i = arg.use_begin(), u_e = arg.use_end(); u_i != u_e; ++u_i) {
      if (isa<StoreInst>(*u_i)) {
        returnSet.insert(*u_i);
        DEBUG_LOG(" Added ARG STORE: " << **u_i << "\n");
        newArg = u_i->getOperand(1);
      }
    }

    ReturnSet alreadyProcessed;
    recursivelyFindAliases(*newArg, returnSet, alreadyProcessed);
}

void FunctionProcessor::recursivelyFindAliases(const Value& arg, ReturnSet& returnSet, ReturnSet& alreadyProcessed) {

  DEBUG_LOG("Recursively find: " << arg << "\n");

  if (Helper::setContains(alreadyProcessed, arg))
    return;

  alreadyProcessed.insert(&arg);

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
      returnSet.insert(&load);
      DOT.addRelation(arg, load, "load");
      DEBUG_LOG(" + Found LOAD for `" << arg.getName() << "` @ " << load << "\n");
      
      recursivelyFindAliases(load, returnSet, alreadyProcessed);
    }
  }
}

void FunctionProcessor::printSet(const TaintSet& s) {
  for (TaintSet::const_iterator i = s.begin(), e = s.end(); i != e; ++i) {
    DEBUG_LOG(**i << " | ");
  }
  
  DEBUG_LOG("\n");
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
          addTaintToSet(taintSet, r);
          DEBUG_LOG(" + Added instruction CONSTANT RETURN VALUE `" << *retval << "`\n");
        } else {
          addTaintToSet(taintSet, *retval);
          DEBUG_LOG(" + Added NON-CONST RETURN VALUE `" << retval << "`\n");
        }

        _returnStatements.insert(make_pair(&r, taintSet));
        DOT.addOutNode(r);
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
