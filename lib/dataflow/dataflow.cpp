#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Analysis/Dominators.h"
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
#include <stdio.h>


using namespace llvm;
using namespace std;

typedef set<Value*> TaintSet;
typedef map<Value*, TaintSet> RetMap;
typedef map<Argument*, TaintSet> ArgMap;

namespace {

  const unsigned int OUTPUT_RELEASE = 1;

  struct Dataflow : public FunctionPass {
    static char ID;

    Dataflow() : FunctionPass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DominatorTree>();
    }

    virtual bool runOnFunction(Function &F) {
      release() << "__taints:";
      release().write_escaped(F.getName()) << "(";

      DominatorTree& DT = getAnalysis<DominatorTree>();

      RetMap returnStatements;
      ArgMap arguments;

      arguments.clear();
      returnStatements.clear();

      printInstructions(F);

      findReturnStatements(F, returnStatements);
      findArguments(F, arguments, returnStatements);

      ArgMap::iterator arg_i = arguments.begin();
      ArgMap::iterator arg_e = arguments.end();
      
      bool isFirstTime = true;

      for(; arg_i != arg_e; ++arg_i) {
        Argument& arg = *arg_i->first;
        TaintSet l = arg_i->second;

        buildArgumentTaintSetFor(F, arg, l, DT);
        intersectSets(arg, l, returnStatements, &isFirstTime);
      }
      release() << ")\n";

      return false;
    }

    private:
    void intersectSets(Argument& arg, TaintSet argTaintSet, RetMap retStmts, bool* isFirstTime) {
      RetMap::iterator ret_i = retStmts.begin();
      RetMap::iterator ret_e = retStmts.end();

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
          release() << (*isFirstTime ? "" : ", ") << arg.getName() << " => " << getValueNameOrDefault(retval);
          debug() << arg.getName() << " => " << getValueNameOrDefault(retval) << "\n";
          *isFirstTime = false;
        }
      }
    }


    void buildArgumentTaintSetFor(Function& F, Argument& arg, TaintSet& taintSet, DominatorTree& DT) {
      debug() << " *** Creating taint set for argument `" << arg.getName() << "`\n";

      for (inst_iterator inst_i = inst_begin(F), inst_e = inst_end(F); inst_i != inst_e; ++inst_i) {
        Instruction& inst = cast<Instruction>(*inst_i);
        //debug() << "Inspecting instruction: " << inst << "\n";

        if (isa<BranchInst>(inst))
          handleBranchInstruction(cast<BranchInst>(inst), taintSet, DT);
        else if (isa<PHINode>(inst))
          debug() << "PHI FOUND!: " << inst << "\n";
        else
          handleInstruction(inst, taintSet, DT);
      }

      debug() << "Taint set for arg `" << arg.getName() << "`:\n";
      printSet(taintSet);
      debug() << "\n";
    }

    void handleBranchInstruction(BranchInst& inst, TaintSet& taintSet, DominatorTree& DT) {
      debug() << "  Inspecting branch instruction:\n";
      if (inst.isConditional()) {
        debug() << "    Cmp is: " << *inst.getCondition() << "\n";
   
        if (taintSet.find(inst.getCondition()) != taintSet.end()) {
          debug() << "    Condition seems tainted.\n";
        } 
      
         
        BasicBlock* brTrue = inst.getSuccessor(0);
        // true branch is always tainted
        taintSet.insert(brTrue);
        debug() << "added true branch to taint set:\n";
        debug() << *brTrue << "\n";

        if (inst.getNumSuccessors() == 2) {
          BasicBlock* brFalse = inst.getSuccessor(1);
          // false branch is only tainted if successor 
          // is not the same as jump target after true branch
          if (getFirstUnconditionalJumpTargetIn(*brTrue) != brFalse) {
            taintSet.insert(brFalse);
            debug() << "added false branch to taint set:\n";
            debug() << *brFalse << "\n";
          }
        }
      }
    }

    BasicBlock* getFirstUnconditionalJumpTargetIn(BasicBlock& block) {
      for (BasicBlock::iterator i = block.begin(), e = block.end(); i != e; ++i) {
        if (isa<BranchInst>(*i)) {
          BranchInst& bi = cast<BranchInst>(*i);
          if (bi.getNumSuccessors() == 1) {
            debug() << " jump-target after true branch: " << *bi.getSuccessor(0) << "\n";
            return bi.getSuccessor(0);
          }
        }
      }

       return NULL;
    }

    void handleInstruction(Instruction& inst, TaintSet& taintSet, DominatorTree& DT) {
      for (TaintSet::iterator s_i = taintSet.begin(), s_e = taintSet.end(); s_i != s_e; ++s_i) {
        if (! isa<BasicBlock>(*s_i))
          continue;

        BasicBlock& taintedBlock = cast<BasicBlock>(**s_i);
        BasicBlock& currentBlock = *inst.getParent();

        //debug() << "inspecting dirty block: " << taintedBlock << "\n";
        if (DT.dominates(&taintedBlock, &currentBlock)) {
          debug() << "dirty block `" << taintedBlock << "` dominates `" << currentBlock << "`\n";
          addValueToSet(taintSet, inst);
          debug() << "instruction tainted by dirty block: " << inst << "\n";
          // Don't care for operand interactions anymore.
          return;
        }
      }

      for (size_t o_i = 0; o_i < inst.getNumOperands(); o_i++) {
         // debug() << "  Inspecting operand #" << o_i << "\n";
          Value& operand = *inst.getOperand(o_i);
          if (taintSet.find(&operand) != taintSet.end()) {
            addValueToSet(taintSet, inst);
            debug() << "    Added " << inst << "\n";
            // Don't care for other operands. Instruction is already tainted.
            break;
          }
        }
    }

    raw_ostream& release() {
      return errs();
    }

    raw_ostream& debug() {
      if (OUTPUT_RELEASE)
        return nulls();

      return errs();
    }

    StringRef getValueNameOrDefault(Value& v) {
      return strlen(v.getName().data()) ? v.getName() : "$_retval";
    }

    void addValueToSet(set<Value*>& l, Instruction& I) {
      if (I.getOpcode() == Instruction::Store)
        l.insert(I.getOperand(1));
      else
        l.insert(&I);
    }

    void findArguments(Function& F, ArgMap& args, RetMap& retStmts) {
      for (Function::arg_iterator i = F.arg_begin(), e = F.arg_end(); i != e; ++i) {
        Argument& arg = *i;
        if (arg.getType()->isPointerTy()) {
          TaintSet retlist;
          findAllStoresAndLoadsForOutArgumentAndAddToSet(F, arg, retlist);
          retStmts.insert(pair<Value*, TaintSet>(&arg, retlist));
          debug() << "added arg `" << arg.getName() << "` to out-list\n";
        }

        TaintSet l;
        l.insert(&arg);
        args.insert(pair<Argument*, TaintSet>(&arg, l));
        debug() << "added arg `" << arg.getName() << "` to arg-list\n";
      }
    }

    void findAllStoresAndLoadsForOutArgumentAndAddToSet(Function& F, Value& arg, TaintSet& retlist) {
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        Instruction& I = cast<Instruction>(*i);
        
        debug() << "inspecting: " << I << "\n";
        if (isa<StoreInst>(I) && I.getOperand(0) == &arg) {
          Value& op = cast<Value>(*I.getOperand(1));
          retlist.insert(&op);
          debug() << "Found store for `" << arg.getName() << "` @ " << op << "\n";

          findAllStoresAndLoadsForOutArgumentAndAddToSet(F, op, retlist);
        }
        
        if (isa<LoadInst>(I) && I.getOperand(0) == &arg) {
          Value& op = cast<Value>(I);
          retlist.insert(&op);
          debug() << "Found load for `" << arg.getName() << "` @ " << op << "\n";
          
          findAllStoresAndLoadsForOutArgumentAndAddToSet(F, op, retlist);
        }
      }
    }

    void printSet(set<Value*>& s) {
      for (TaintSet::iterator i = s.begin(), e = s.end(); i != e; ++i) {
        debug() << **i << " | ";
      }
    } 
    void findReturnStatements(Function& F, RetMap& retStmts) {
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
          if (isa<ReturnInst>(*i)) {
            ReturnInst& r = cast<ReturnInst>(*i);
            TaintSet l;
            
            // skip 'return void'
            if (r.getReturnValue()) {
              l.insert(r.getReturnValue());
              retStmts.insert(pair<Value*, set<Value*> >(r.getReturnValue(), l));
              debug() << "Found ret-stmt: " << r << "\n";
            }
          }
      }
    }

    void printInstructions(Function &F) {
      debug() << "Instructions: \n";

      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        debug() << i->getParent() << " | " << (*i) << "\n";
      }
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> X("dataflow", "Data-flow analysis", true, true);
