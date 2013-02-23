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
#include <map>
#include <set>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include "FunctionProcessor.h"


#define STOP_ON_CANCEL if (canceledInspection) return;

using namespace llvm;
using namespace std;

bool FunctionProcessor::didFinish() {
  return !canceledInspection;
}

void FunctionProcessor::processFunction() {
  printInstructions();

  findReturnStatements();
  findArguments();

  ArgMap::iterator arg_i = _arguments.begin();
  ArgMap::iterator arg_e = _arguments.end();
      
  for(; arg_i != arg_e; ++arg_i) {
    Argument& arg = *arg_i->first;
    TaintSet taintSet = arg_i->second;

    int iteration = 0;
    int newSetLength;
    int oldSetLength;

    do {
      oldSetLength = taintSet.size();

      debug() << " ** Begin Iteration #" << iteration << "\n";
      buildTaintSetFor(arg, taintSet);
      STOP_ON_CANCEL
      debug() << " ** End Iteration #" << iteration << "\n";

      newSetLength = taintSet.size();
      debug() << " ** Taint set length:" << newSetLength << "\n";

      iteration++;
    } while (iteration < 10 && oldSetLength != newSetLength);

    STOP_ON_CANCEL

    intersectSets(arg, taintSet);
  }

  printTaints();
}

void FunctionProcessor::intersectSets(Argument& arg, TaintSet argTaintSet) {
  RetMap::iterator ret_i = _returnStatements.begin();
  RetMap::iterator ret_e = _returnStatements.end();

  for (; ret_i != ret_e; ++ret_i) {
    Value& retval = *ret_i->first;
    TaintSet retTaintSet = ret_i->second;

    if (&retval == &arg) {
      debug() << "Skipping detected self-taint\n";
      continue;
    }

    debug() << "Ret-set for `" << retval << "`:\n",
    printSet(retTaintSet);
    debug() << "\n";

    TaintSet intersect;
    set_intersection(argTaintSet.begin(), argTaintSet.end(), retTaintSet.begin(), retTaintSet.end(), 
                     inserter(intersect, intersect.end()));

    if (intersect.begin() != intersect.end()) {
      addTaint(arg, retval);
    }
  }
}


void FunctionProcessor::buildTaintSetFor(Value& arg, TaintSet& taintSet) {
  debug() << " *** Creating taint set for argument `" << arg.getName() << "`\n";

  // Arg trivially taints itself.
  taintSet.insert(&arg);

  for (Function::iterator b_i = F.begin(), b_e = F.end(); b_i != b_e; ++b_i) {
    STOP_ON_CANCEL

    BasicBlock& block = cast<BasicBlock>(*b_i);
    processBasicBlock(block, taintSet);
  }

  debug() << "Taint set for arg `" << arg.getName() << "`:\n";
  printSet(taintSet);
  debug() << "\n";
}

void FunctionProcessor::addTaint(Argument& tainter, Value& taintee) {
  _taints.insert(TaintPair(&tainter, &taintee));
}

bool FunctionProcessor::setContains(TaintSet& taintSet, Value& val) {
  return taintSet.find(&val) != taintSet.end();
}

void FunctionProcessor::processBasicBlock(BasicBlock& block, TaintSet& taintSet) {
  for (BasicBlock::iterator inst_i = block.begin(), inst_e = block.end(); inst_i != inst_e; ++inst_i) {
    STOP_ON_CANCEL

    Instruction& inst = cast<Instruction>(*inst_i);
    debug() << "Inspecting instruction: " << inst << "\n";

    if (isa<BranchInst>(inst))
      handleBranchInstruction(cast<BranchInst>(inst), taintSet);
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
  }
}

void FunctionProcessor::printTaints() {
  release() << "__taints:";
  release().write_escaped(F.getName()) << "(";
  bool isFirstTime = true;

  for (ResultSet::iterator i = _taints.begin(), e = _taints.end(); i != e; ++i) {
    Argument& arg = cast<Argument>(*i->first);
    Value& retval = cast<Value>(*i->second);

    release() << (isFirstTime ? "" : ", ") << arg.getName() << " => " << getValueNameOrDefault(retval);
    isFirstTime = false;
  }

  release() << ")\n";
}

void FunctionProcessor::handleGetElementPtrInstruction(GetElementPtrInst& inst, TaintSet& taintSet) {
  Value& op = *inst.getPointerOperand();
  if (setContains(taintSet, op)) {
    taintSet.insert(&inst); 
    DOT->addRelation(op, inst, "indexer");
    debug() << " + Added GEP taint: `" << inst << "`\n";
  }
}

void FunctionProcessor::handleStoreInstruction(StoreInst& storeInst, TaintSet& taintSet) {
  Value& source = *storeInst.getOperand(0);
  Value& target = *storeInst.getOperand(1);

  debug() << " Handle STORE instruction " << storeInst << "\n";
  if (setContains(taintSet, source) || handleBlockTainting(taintSet, storeInst)) {
    taintSet.insert(&target);
    taintSet.insert(&storeInst);
    DOT->addRelation(source, target, "store");
    debug() << " + Added STORE taint: " << source << " --> " << target << "\n";
    if (isa<GetElementPtrInst>(target)) {
      GetElementPtrInst& gep = cast<GetElementPtrInst>(target);
      for (User::op_iterator gep_i = gep.idx_begin(), gep_e = gep.idx_end(); gep_i != gep_e; ++gep_i) {
        Value& gepOperand = *cast<Value>(*gep_i);
        taintSet.insert(&gepOperand);
        debug() << " ++ Also added GEP `" << gepOperand << "` because it is used by STORE.\n";
      }
    }
  } else if (setContains(taintSet, target) && isCfgSuccessorOfPreviousStores(storeInst, taintSet)) {
    // Only do removal if value is really in set
    taintSet.erase(&target);
    debug() << " - Removed STORE taint due to non-tainted overwrite: " << source << " --> " << target << "\n";

    if (isa<LoadInst>(target)) {
      Value* load = (cast<LoadInst>(target)).getPointerOperand();
      taintSet.erase(load);
      debug() << " - Also removed transitive LOAD." << *load << "\n";
    }

    DOT->addRelation(source, target, "non-taint overwrite");
  }
}

bool FunctionProcessor::isCfgSuccessorOfPreviousStores(StoreInst& storeInst, TaintSet& taintSet) {
  debug() << " CFG SUCC: start for " << storeInst << "\n";
  for (TaintSet::iterator i = taintSet.begin(), e = taintSet.end(); i != e; ++i) {
    debug() << " CFG SUCC: inspecting " << **i << "\n";

    if (!isa<StoreInst>(*i))
      continue;

    StoreInst& prevStore = *cast<StoreInst>(*i);

    debug() << " CFG SUCC: testing storeInst Operand: " << *storeInst.getOperand(1) << "\n";
    debug() << " CFG SUCC: testing prevStore Operand: " << *prevStore.getOperand(1) << "\n";

    if (prevStore.getOperand(1) != storeInst.getOperand(1))
      continue;

    debug() << " CFG SUCC: testing storeInst: " << prevStore << "\n";
    set<BasicBlock*> usedList;
    if (!isCfgSuccessor(storeInst.getParent(), prevStore.getParent(), usedList)) {
      debug() << " CFG SUCC: in-if: " << prevStore << "\n";
      return false;
    }
  }

  return true;
}

bool FunctionProcessor::isCfgSuccessor(BasicBlock* succ, BasicBlock* pred, set<BasicBlock*>& usedList) {
  if (NULL == succ || NULL == pred)
    return false;

  if (pred == succ)
    return true;

  for (pred_iterator i = pred_begin(succ), e = pred_end(succ); i != e; ++i) {
    if (usedList.count(succ))
      continue;
  debug() << *i << "\n";

    usedList.insert(*i);
    if (isCfgSuccessor(*i, pred, usedList))
      return true;
  }

  return false;
}

void FunctionProcessor::readTaintsFromFile(TaintSet& taintSet, CallInst& callInst, Function& func, ResultSet& result) {
  ifstream file((func.getName().str() + ".taints").c_str(), ios::in);
  if (!file.is_open()) {
    debug() << " -- Cannot get information about `" << func.getName() << "` -- cancel.";
    canceledInspection = true;
    return;
  }

  string line;
  while (file.good()) {
    getline(file, line);
    
    string paramName;
    string valName;
    istringstream iss(line);

    iss >> paramName;
    // consume => :
    iss >> valName;
    iss >> valName;

    int paramPos;
    int retvalPos;
    int i = 0;

    stringstream convert1(paramName);
    if( !(convert1 >> paramPos)) {
      paramPos = -1;
      debug() << "Searching for param " << paramName << "\n";
      for (Function::arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (a_i->getName().str() == paramName) {
          paramPos = i;
          debug() << "Found at #" << i << "\n";
          break;
        }

        i++;
      }
    } else {
      debug() << "Param-info from file: seem to be at #" << paramPos << "\n";
    }

    i = 0;
    stringstream convert2(valName);
    if( !(convert2 >> retvalPos)) {
      retvalPos = -1;
      debug() << "Searching for retval " << valName << "\n";
      for (Function::arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (a_i->getName().str() == valName) {
          retvalPos = i;
          debug() << "Found at #" << i << "\n";
          break;
        }

        i++;
      }
    } else {
      debug() << "Retval-info from file: seem to be at #" << retvalPos << "\n";
    }

    if (paramPos == -1) {
      debug() << "  - Skipping `" << paramName << "` -- not found.\n";
      continue;
    }

    Value* arg = callInst.getArgOperand(paramPos);

    if (setContains(taintSet, *arg)) {
      stringstream reas("");
      reas << "in, arg#" << paramPos;

      DOT->addCallNode(func);
      DOT->addRelation(*arg, func, reas.str());

      if (retvalPos == -1) {
        debug() << " + Added retval taint `" << callInst << "`\n";
        taintSet.insert(&callInst);
        DOT->addRelation(func, callInst, "ret");
      }
      else {
        Value* returnTarget = callInst.getArgOperand(retvalPos);
        debug() << " + Added out-argument taint `" << returnTarget->getName() << "`\n";
        taintSet.insert(returnTarget);
        stringstream outReas("");
        outReas << "out, arg#" << retvalPos;
        DOT->addRelation(func, *returnTarget, outReas.str());

        // Value is a pointer, so the previous load is also tainted.
        if (isa<LoadInst>(returnTarget)) {
          Value* op = (cast<LoadInst>(returnTarget))->getOperand(0);
          taintSet.insert(op);
          debug() << " ++ Added previous load: " << *returnTarget << "\n";
          DOT->addRelation(*op, *returnTarget, "load");
        }
      }

      debug() << "  - Argument `" << *arg << "` taints f-param at pos #" << paramPos << "\n";
    }
  }

  file.close();
}

void FunctionProcessor::handleCallInstruction(CallInst& callInst, TaintSet& taintSet) {
  debug() << " Handle CALL instruction:\n";
  Function* callee = callInst.getCalledFunction();

  if (callee != NULL) {
    debug() << " * calling function `" << *callee << "`\n";

    ResultSet result;
    readTaintsFromFile(taintSet, callInst, *callee, result);
    
  } else {
    debug() << " ! cannot get information about callee `" << *callInst.getCalledValue() << "`\n";
  }
}

void FunctionProcessor::handleSwitchInstruction(SwitchInst& inst, TaintSet& taintSet) {
  Value* condition = inst.getCondition();
  
  if (!setContains(taintSet, *condition))
    return;

  DOT->addRelation(*condition, inst, "switch");

  debug() << " Handle SWITCH instruction:\n";
  for (size_t i = 0; i < inst.getNumSuccessors(); ++i) {
    // Mark all case-blocks as tainted.
    BasicBlock& caseBlock = *inst.getSuccessor(i);
    DOT->addBlockNode(caseBlock);
    DOT->addRelation(inst, caseBlock, "case");
    taintSet.insert(&caseBlock);
    debug() << " + Added Block due to tainted SWITCH condition: " << caseBlock << "\n";
  }
}

void FunctionProcessor::handleBranchInstruction(BranchInst& inst, TaintSet& taintSet) {
  debug() << " Handle BRANCH instruction:\n";
  BasicBlock& currentBlock = *inst.getParent();
  
  // Process all nested blocks to find possible tainting of 
  // condition expression.
  DomTreeNode* node = DT.getNode(&currentBlock);
  if (node != NULL) {
    for (DomTreeNode::iterator n_i = node->begin(), n_e = node->end(); n_i != n_e; ++n_i) {
      BasicBlock& childBlock = *(*n_i)->getBlock();
      debug() << " ****  Begin: Processing child block " << childBlock << "\n";
      processBasicBlock(childBlock, taintSet);
      debug() << " ****  End: Processing child block\n";
    }
  } else {
    debug() << " **** ! Block not in DT: " << currentBlock << "\n";
  }

  if (inst.isConditional()) {
    Instruction& cmp = cast<Instruction>(*inst.getCondition());
    debug() << " = Compare instruction is: " << cmp << "\n";

    bool isConditionTainted = setContains(taintSet, cmp);
    if (isConditionTainted) {
      DOT->addRelation(cmp, inst, "condition");
     
      BasicBlock* brTrue = inst.getSuccessor(0);
      // true branch is always tainted
      taintSet.insert(brTrue);
      DOT->addBlockNode(*brTrue);
      DOT->addRelation(inst, *brTrue, "br-true");
      debug() << " + Added TRUE branch to taint set:\n";
      debug() << *brTrue << "\n";

      if (inst.getNumSuccessors() == 2) {
        BasicBlock* brFalse = inst.getSuccessor(1);
        // false branch is only tainted if successor 
        // is not the same as jump target after true branch
        BasicBlock* target = PDT.findNearestCommonDominator(brFalse, brTrue);
        debug() << "   Nearest Common Post-Dominator for tr/fa: " << *target << "\n";

        if (target != brFalse) {
          taintSet.insert(brFalse);
          DOT->addBlockNode(*brFalse);
          DOT->addRelation(inst, *brFalse, "br-false");
          debug() << " + Added FALSE branch to taint set:\n";
          debug() << *brFalse << "\n";
        }
      }
    }
  }

  handleBlockTainting(taintSet, inst);
}

void FunctionProcessor::handleInstruction(Instruction& inst, TaintSet& taintSet) {
  debug() << " Handle OTHER instruction:\n";

  for (size_t o_i = 0; o_i < inst.getNumOperands(); o_i++) {
     debug() << " ~ Inspecting operand #" << o_i << "\n";
     Value& operand = *inst.getOperand(o_i);
     if (setContains(taintSet, operand)) {
       taintSet.insert(&inst);
       DOT->addRelation(operand, inst, string(Instruction::getOpcodeName(inst.getOpcode())));

       debug() << " + Added " << operand << " --> " << inst << "\n";
    }
  }

  handleBlockTainting(taintSet, inst);
}

bool FunctionProcessor::handleBlockTainting(TaintSet& taintSet, Instruction& inst) {
  debug() << " Handle BLOCK-tainting for " << inst << "\n";
  bool result = false;

  for (TaintSet::iterator s_i = taintSet.begin(), s_e = taintSet.end(); s_i != s_e; ++s_i) {
    if (! isa<BasicBlock>(*s_i))
      continue;

    BasicBlock& taintedBlock = cast<BasicBlock>(**s_i);
    BasicBlock& currentBlock = *inst.getParent();

    if (DT.dominates(&taintedBlock, &currentBlock)) {
      debug() << " ! Dirty block `" << taintedBlock.getName() << "` dominates `" << currentBlock.getName() << "`\n";

      if (&taintedBlock != &currentBlock) {
        taintSet.insert(&currentBlock);
        DOT->addBlockNode(currentBlock);
        DOT->addRelation(taintedBlock, currentBlock, "block-taint");
      }

      taintSet.insert(&inst);
      DOT->addRelation(currentBlock, inst, "block-taint");

      debug() << " + Instruction tainted by dirty block: " << inst << "\n";
      result = true;
    }
  }

  return result;
}

StringRef FunctionProcessor::getValueNameOrDefault(Value& v) {
  if (isa<Argument>(v))
    return v.getName();
  else
    return "$_retval";
}

void FunctionProcessor::findArguments() {
  for (Function::arg_iterator i = F.arg_begin(), e = F.arg_end(); i != e; ++i) {
    Argument& arg = *i;
    bool isInOutNode = false;

    if (arg.getType()->isPointerTy()) {
      TaintSet retlist;
      findAllStoresAndLoadsForOutArgumentAndAddToSet(arg, retlist);
      _returnStatements.insert(pair<Value*, TaintSet>(&arg, retlist));
      DOT->addInOutNode(arg);
      isInOutNode = true;

      debug() << "added arg `" << arg.getName() << "` to out-list\n";
    }

    TaintSet taintSet;
    taintSet.insert(&arg);
    if (!isInOutNode)
      DOT->addInNode(arg);

    _arguments.insert(pair<Argument*, TaintSet>(&arg, taintSet));
    debug() << "added arg `" << arg.getName() << "` to arg-list\n";
  }
}

void FunctionProcessor::findAllStoresAndLoadsForOutArgumentAndAddToSet(Value& arg, TaintSet& retlist) {
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    Instruction& I = cast<Instruction>(*i);
    
    debug() << "inspecting: " << I << "\n";
    if (isa<StoreInst>(I) && I.getOperand(0) == &arg) {
      Value& op = cast<Value>(*I.getOperand(1));
      retlist.insert(&op);
      DOT->addRelation(arg, op, "store");
      DOT->addRelation(op, arg, "out-init");
      debug() << " + Found STORE for `" << arg.getName() << "` @ " << op << "\n";

      findAllStoresAndLoadsForOutArgumentAndAddToSet(op, retlist);
    }
    
    if (isa<GetElementPtrInst>(I)) {
      Value& ptr = *(cast<GetElementPtrInst>(I)).getPointerOperand();
      if (!setContains(retlist, ptr))
        continue;

      retlist.insert(&I);
      debug() << " + Found GEP for `" << arg.getName() << "` @ " << ptr << "\n";
      DOT->addRelation(arg, ptr, "gep");
    }

    if (isa<LoadInst>(I) && I.getOperand(0) == &arg) {
      Value& op = cast<Value>(I);
      retlist.insert(&op);
      DOT->addRelation(op, arg, "load");
      debug() << " + Found LOAD for `" << arg.getName() << "` @ " << op << "\n";
      
      findAllStoresAndLoadsForOutArgumentAndAddToSet(op, retlist);
    }
  }
}

void FunctionProcessor::printSet(set<Value*>& s) {
  for (TaintSet::iterator i = s.begin(), e = s.end(); i != e; ++i) {
    debug() << **i << " | ";
  }
} 

void FunctionProcessor::findReturnStatements() {
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    if (isa<ReturnInst>(*i)) {
      ReturnInst& r = cast<ReturnInst>(*i);
      TaintSet taintSet;
      
      // skip 'return void'
      Value* retval = r.getReturnValue();
      if (retval) {
        taintSet.insert(retval);
        _returnStatements.insert(pair<Value*, set<Value*> >(retval, taintSet));
        DOT->addOutNode(r);
        debug() << "Found ret-stmt: " << r << "\n";
      }
    }
  }
}

void FunctionProcessor::printInstructions() {
  debug() << "Instructions: \n";

  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    debug() << i->getParent() << " | " << &*i << " | " << (*i) << "\n";
  }
}
