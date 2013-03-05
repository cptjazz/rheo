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

  TaintMap::iterator arg_i = _arguments.begin();
  TaintMap::iterator arg_e = _arguments.end();
      
  for(; arg_i != arg_e; ++arg_i) {
    Value& arg = *arg_i->first;
    TaintSet taintSet = arg_i->second;
    TaintSet oldTaintSet;

    int iteration = 0;

    do {
      oldTaintSet = taintSet;

      debug() << " ** Begin Iteration #" << iteration << "\n";
      buildTaintSetFor(arg, taintSet);
      STOP_ON_CANCEL
      debug() << " ** End Iteration #" << iteration << "\n";

      debug() << " ** Taint set length:" << taintSet.size() << "\n";

      iteration++;
    } while (iteration < 10 && !Helper::areSetsEqual(oldTaintSet, taintSet));

    STOP_ON_CANCEL

    intersectSets(arg, taintSet);
  }

  printTaints();
}

void FunctionProcessor::intersectSets(Value& arg, TaintSet argTaintSet) {
  TaintMap::iterator ret_i = _returnStatements.begin();
  TaintMap::iterator ret_e = _returnStatements.end();

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

    if (intersect.size()) {
      addTaint(arg, retval);

      debug() << "Values that lead to taint "
              << Helper::getValueNameOrDefault(arg) << " -> "
              << Helper::getValueNameOrDefault(retval) << ":\n";
      for (TaintSet::iterator i_i = intersect.begin(), i_e = intersect.end(); i_i != i_e; ++i_i)
        debug() << **i_i << "\n";
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

void FunctionProcessor::addTaint(Value& tainter, Value& taintee) {
  _taints.insert(make_pair(&tainter, &taintee));
}

void FunctionProcessor::processBasicBlock(BasicBlock& block, TaintSet& taintSet) {
  for (BasicBlock::iterator inst_i = block.begin(), inst_e = block.end(); inst_i != inst_e; ++inst_i) {
    STOP_ON_CANCEL

    Instruction& inst = cast<Instruction>(*inst_i);
//    debug() << "Inspecting instruction: " << inst << "\n";

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
    Value& arg = cast<Value>(*i->first);
    Value& retval = cast<Value>(*i->second);

    release() << (isFirstTime ? "" : ", ") << arg.getName() << " => " << Helper::getValueNameOrDefault(retval);
    isFirstTime = false;
  }

  release() << ")\n";
}

void FunctionProcessor::handleGetElementPtrInstruction(GetElementPtrInst& inst, TaintSet& taintSet) {
  Value& op = *inst.getPointerOperand();
  if (Helper::setContains(taintSet, op)) {
    taintSet.insert(&inst); 
    DOT.addRelation(op, inst, "indexer");
    debug() << " + Added GEP taint: `" << inst << "`\n";
  }
}

void FunctionProcessor::handleStoreInstruction(StoreInst& storeInst, TaintSet& taintSet) {
  Value& source = *storeInst.getOperand(0);
  Value& target = *storeInst.getOperand(1);

  debug() << " Handle STORE instruction " << storeInst << "\n";
  if (Helper::setContains(taintSet, source) || handleBlockTainting(taintSet, storeInst)) {
    taintSet.insert(&target);
    taintSet.insert(&storeInst);
    DOT.addRelation(source, target, "store");
    debug() << " + Added STORE taint: " << source << " --> " << target << "\n";
    if (isa<GetElementPtrInst>(target)) {
      GetElementPtrInst& gep = cast<GetElementPtrInst>(target);
      recursivelyAddAllGeps(gep, taintSet);
    }
  } else if (Helper::setContains(taintSet, target) && isCfgSuccessorOfPreviousStores(storeInst, taintSet)) {
    // Only do removal if value is really in set
    taintSet.erase(&target);
    debug() << " - Removed STORE taint due to non-tainted overwrite: " << source << " --> " << target << "\n";

    if (isa<LoadInst>(target)) {
      Value* load = (cast<LoadInst>(target)).getPointerOperand();
      taintSet.erase(load);
      debug() << " - Also removed transitive LOAD." << *load << "\n";
    }

    DOT.addRelation(source, target, "non-taint overwrite");
  }
}

void FunctionProcessor::recursivelyAddAllGeps(GetElementPtrInst& gep, TaintSet& taintSet) {
  Value& ptrOp = *gep.getPointerOperand();
  debug() << " ++ Added GEP SOURCE:" << ptrOp << "\n";
  taintSet.insert(&ptrOp);
  DOT.addRelation(gep, ptrOp);

  for (User::op_iterator gep_i = gep.idx_begin(), gep_e = gep.idx_end(); gep_i != gep_e; ++gep_i) {
    // Skip constant array indices
    if (isa<Constant>(*gep_i))
      continue;

    Value& gepOperand = *cast<Value>(*gep_i);
    taintSet.insert(&gepOperand);
    debug() << " ++ Also added GEP INDEX `" << gepOperand << "` because it is used by STORE.\n";
  }

  //if (isa<GetElementPtrInst>(ptrOp))
   // recursivelyAddAllGeps(ptrOp, taintSet);
}

bool FunctionProcessor::isCfgSuccessorOfPreviousStores(StoreInst& storeInst, TaintSet& taintSet) {
//  debug() << " CFG SUCC: start for " << storeInst << "\n";
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
    file.open(("taintlib/" + func.getName().str() + ".taints").c_str(), ios::in);

    if (!file.is_open()) {
      debug() << " -- Cannot get information about `" << func.getName() << "` -- cancel.";
      canceledInspection = true;
      return;
    }
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

    if (Helper::setContains(taintSet, *arg)) {
      stringstream reas("");
      reas << "in, arg#" << paramPos;

      DOT.addCallNode(func);
      DOT.addRelation(*arg, func, reas.str());

      if (retvalPos == -1) {
        debug() << " + Added retval taint `" << callInst << "`\n";
        taintSet.insert(&callInst);
        DOT.addRelation(func, callInst, "ret");
      }
      else {
        Value* returnTarget = callInst.getArgOperand(retvalPos);
        debug() << " + Added out-argument taint `" << returnTarget->getName() << "`\n";
        taintSet.insert(returnTarget);
        stringstream outReas("");
        outReas << "out, arg#" << retvalPos;
        DOT.addRelation(func, *returnTarget, outReas.str());

        // Value is a pointer, so the previous load is also tainted.
        if (isa<LoadInst>(returnTarget)) {
          Value* op = (cast<LoadInst>(returnTarget))->getOperand(0);
          taintSet.insert(op);
          debug() << " ++ Added previous load: " << *returnTarget << "\n";
          DOT.addRelation(*op, *returnTarget, "load");
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
  
  if (!Helper::setContains(taintSet, *condition))
    return;

  DOT.addRelation(*condition, inst, "switch");

  debug() << " Handle SWITCH instruction:\n";
  for (size_t i = 0; i < inst.getNumSuccessors(); ++i) {
    // Mark all case-blocks as tainted.
    BasicBlock& caseBlock = *inst.getSuccessor(i);
    DOT.addBlockNode(caseBlock);
    DOT.addRelation(inst, caseBlock, "case");
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

    bool isConditionTainted = Helper::setContains(taintSet, cmp);
    if (isConditionTainted) {
      DOT.addRelation(cmp, inst, "condition");
     
      BasicBlock* brTrue = inst.getSuccessor(0);
      // true branch is always tainted
      taintSet.insert(brTrue);
      DOT.addBlockNode(*brTrue);
      DOT.addRelation(inst, *brTrue, "br-true");
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
          DOT.addBlockNode(*brFalse);
          DOT.addRelation(inst, *brFalse, "br-false");
          debug() << " + Added FALSE branch to taint set:\n";
          debug() << *brFalse << "\n";
        }
      }
    }
  }

  handleBlockTainting(taintSet, inst);
}

void FunctionProcessor::handleInstruction(Instruction& inst, TaintSet& taintSet) {
//  debug() << " Handle OTHER instruction:\n";

  for (size_t o_i = 0; o_i < inst.getNumOperands(); o_i++) {
 //    debug() << " ~ Inspecting operand #" << o_i << "\n";
     Value& operand = *inst.getOperand(o_i);
     if (Helper::setContains(taintSet, operand)) {
       taintSet.insert(&inst);
       DOT.addRelation(operand, inst, string(Instruction::getOpcodeName(inst.getOpcode())));

       debug() << " + Added " << operand << " --> " << inst << "\n";
    }
  }

  handleBlockTainting(taintSet, inst);
}

bool FunctionProcessor::handleBlockTainting(TaintSet& taintSet, Instruction& inst) {
  debug() << " Handle BLOCK-tainting for " << inst << "\n";
  bool result = false;

  // Loads should not be tained by parenting block
  // because otherwise a block would taint a load of
  // a global variable what makes no sense -- it would
  // introduce a taint that does not exist.
  if (isa<LoadInst>(inst)) {
    debug() << " Ignoring LOAD\n";
    return false;
  }

  if (isa<GetElementPtrInst>(inst)) {
    debug() << " Ignoring GEP\n";
    return false;
  }

  for (TaintSet::iterator s_i = taintSet.begin(), s_e = taintSet.end(); s_i != s_e; ++s_i) {
    if (! isa<BasicBlock>(*s_i))
      continue;

    BasicBlock& taintedBlock = cast<BasicBlock>(**s_i);
    BasicBlock& currentBlock = *inst.getParent();

    if (DT.dominates(&taintedBlock, &currentBlock)) {
      debug() << " ! Dirty block `" << taintedBlock.getName() << "` dominates `" << currentBlock.getName() << "`\n";

      if (&taintedBlock != &currentBlock) {
        taintSet.insert(&currentBlock);
        DOT.addBlockNode(currentBlock);
        DOT.addRelation(taintedBlock, currentBlock, "block-taint");
      }

      taintSet.insert(&inst);
      DOT.addRelation(currentBlock, inst, "block-taint");

      debug() << " + Instruction tainted by dirty block: " << inst << "\n";
      result = true;
    }
  }

  return result;
}

void FunctionProcessor::findArguments() {
  for (Function::arg_iterator a_i = F.arg_begin(), a_e = F.arg_end(); a_i != a_e; ++a_i) {
    Argument& arg = *a_i;
    handleFoundArgument(arg);
  }

  for (Module::global_iterator m_i = M.global_begin(), m_e = M.global_end(); m_i != m_e; ++m_i) {
    GlobalVariable& g = *m_i;
    handleFoundArgument(g);
  }
}

void FunctionProcessor::handleFoundArgument(Value& arg) {
  bool isInOutNode = false;

  if (arg.getType()->isPointerTy() || isa<GlobalVariable>(arg)) {
    TaintSet retlist;
    retlist.insert(&arg);

    findAllStoresAndLoadsForOutArgumentAndAddToSet(arg, retlist);

    _returnStatements.insert(make_pair(&arg, retlist));
    DOT.addInOutNode(arg);
    isInOutNode = true;

    debug() << "added arg `" << arg.getName() << "` to out-list\n";
  }

  TaintSet taintSet;
  taintSet.insert(&arg);
  if (!isInOutNode)
    DOT.addInNode(arg);

  _arguments.insert(make_pair(&arg, taintSet));
  debug() << "added arg `" << arg.getName() << "` to arg-list\n";
}

void FunctionProcessor::findAllStoresAndLoadsForOutArgumentAndAddToSet(Value& arg, TaintSet& taintSet) {

    Value* newArg = &arg;

    for (User::use_iterator u_i = arg.use_begin(), u_e = arg.use_end(); u_i != u_e; ++u_i) {
      if (isa<StoreInst>(*u_i)) {
        taintSet.insert(*u_i);
        debug() << " Added ARG STORE: " << **u_i << "\n";
        newArg = u_i->getOperand(1);
      }
    }

    TaintSet alreadyProcessed;
    recursivelyFindAliases(*newArg, taintSet, alreadyProcessed);
}

void FunctionProcessor::recursivelyFindAliases(Value& arg, TaintSet& taintSet, TaintSet& alreadyProcessed) {

  debug() << "Recursively find: " << arg << "\n";

  if (Helper::setContains(alreadyProcessed, arg))
    return;

  alreadyProcessed.insert(&arg);

  for (User::use_iterator i = arg.use_begin(), e = arg.use_end(); i != e; ++i) {
    if (!isa<Instruction>(**i)) {
      // Necessary, as constant string literals also come 
      // as GlobalVariables, but do not point to an instruction.
      // They point to an operand of a GEP.
      debug() << "Skip: " << **i << "\n";
      continue;
    }

    Instruction& I = cast<Instruction>(**i);
    
    debug() << "Inspecting: " << I << "\n";

    if (isa<GetElementPtrInst>(I)) {
      GetElementPtrInst& gep = cast<GetElementPtrInst>(I);
      Value& ptr = *gep.getPointerOperand();

      if (&arg != &ptr)
        continue;

      taintSet.insert(&I);
      debug() << " + Found GEP for `" << arg.getName() << "` @ " << gep << "\n";
      DOT.addRelation(arg, ptr, "gep");

      recursivelyFindAliases(gep, taintSet, alreadyProcessed);
   }

    if (isa<LoadInst>(I) && I.getOperand(0) == &arg) {
      Value& op = cast<Value>(I);
      taintSet.insert(&op);
      DOT.addRelation(op, arg, "load");
      debug() << " + Found LOAD for `" << arg.getName() << "` @ " << op << "\n";
      
      recursivelyFindAliases(op, taintSet, alreadyProcessed);
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
        if (isa<Constant>(retval)) {
          taintSet.insert(&r);
          debug() << " + Added instruction CONSTANT RETURN VALUE `" << retval << "`\n";
        } else {
          taintSet.insert(retval);
          debug() << " + Added NON-CONST RETURN VALUE `" << retval << "`\n";
        }

        if (isa<PHINode>(retval)) {
          PHINode& phi = cast<PHINode>(*retval);
          for (size_t j = 0; j < phi.getNumIncomingValues(); ++j) {
            debug() << " + Added PHI block" << *phi.getIncomingBlock(j) << "\n";
            taintSet.insert(phi.getIncomingBlock(j));
          }
        }

        _returnStatements.insert(make_pair(&r, taintSet));
        DOT.addOutNode(r);
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
