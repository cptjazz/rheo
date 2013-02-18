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
      debug() << " ** End Iteration #" << iteration << "\n";

      newSetLength = taintSet.size();
      debug() << " ** Taint set length:" << newSetLength << "\n";

      iteration++;
    } while (iteration < 10 && oldSetLength != newSetLength);

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
    Instruction& inst = cast<Instruction>(*inst_i);
    debug() << "Inspecting instruction: " << inst << "\n";

    if (isa<BranchInst>(inst))
      handleBranchInstruction(cast<BranchInst>(inst), taintSet);
    else if (isa<StoreInst>(inst))
      handleStoreInstruction(cast<StoreInst>(inst), taintSet);
    else if (isa<CallInst>(inst))
      handleCallInstruction(cast<CallInst>(inst), taintSet);
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

void FunctionProcessor::handleStoreInstruction(StoreInst& storeInst, TaintSet& taintSet) {
  Value& source = *storeInst.getOperand(0);
  Value& target = *storeInst.getOperand(1);

  debug() << " Handle STORE instruction " << storeInst << "\n";
  if (setContains(taintSet, source) || handleBlockTainting(taintSet, storeInst)) {
    taintSet.insert(&target);
    DOT->addRelation(source, target);
    debug() << " + Added STORE taint: " << source << " --> " << target << "\n";
  } else if (setContains(taintSet, target)) {
    // Only do removal if value is really in set
    taintSet.erase(&target);
    debug() << " - Removed STORE taint due to non-tainted overwrite: " << source << " --> " << target << "\n";
    DOT->addRelation(source, target, "non-taint overwrite");
  }
}

void FunctionProcessor::readTaintsFromFile(Function& func, ResultSet& result) {
  ifstream file((func.getName().str() + ".taints").c_str(), ios::in);
  if (!file.is_open()) {
    debug() << " -- No file -- cancel.";
    return;
  }

  string line;
  while (file.good()) {
    getline(file, line);
    
    string argName;
    string valName;
    istringstream iss(line);

    iss >> argName;
    // consume => :
    iss >> valName;
    iss >> valName;

    Argument* arg;
    for (Function::arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
      arg = &*a_i;
      if (arg->getName().str() == argName)
        break; 
    }
    
    debug() << "Found arg: " << *arg << "\n";
  }
}

void FunctionProcessor::handleCallInstruction(CallInst& callInst, TaintSet& taintSet) {
  debug() << " Handle CALL instruction:\n";
  Function* callee = callInst.getCalledFunction();

  if (callee != NULL) {
    debug() << " * calling function `" << *callee << "`\n";

    ResultSet result;
    readTaintsFromFile(*callee, result);
    
  } else {
    debug() << " ! cannot get information about callee `" << *callInst.getCalledValue() << "`\n";
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
      DOT->addRelation(cmp, inst);
     
      BasicBlock* brTrue = inst.getSuccessor(0);
      // true branch is always tainted
      taintSet.insert(brTrue);
      DOT->addRelation(inst, *brTrue);
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
          DOT->addRelation(inst, *brFalse);
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
       DOT->addRelation(operand, inst);

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

      taintSet.insert(&currentBlock);
      DOT->addRelation(taintedBlock, currentBlock);

      taintSet.insert(&inst);
      DOT->addRelation(currentBlock, inst);

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
      DOT->addRelation(arg, op);
      debug() << "Found store for `" << arg.getName() << "` @ " << op << "\n";

      findAllStoresAndLoadsForOutArgumentAndAddToSet(op, retlist);
    }
    
    if (isa<LoadInst>(I) && I.getOperand(0) == &arg) {
      Value& op = cast<Value>(I);
      retlist.insert(&op);
      DOT->addRelation(op, arg);
      debug() << "Found load for `" << arg.getName() << "` @ " << op << "\n";
      
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
