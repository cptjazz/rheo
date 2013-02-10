#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
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

  const unsigned int OUTPUT_RELEASE = 0;

  struct Dataflow : public FunctionPass {
    static char ID;

    RetMap returnStatements;
    ArgMap arguments;

    Dataflow() : FunctionPass(ID) { }

    virtual bool runOnFunction(Function &F) {
      bool isFirstTime = true;

      release() << "__taints:";
      release().write_escaped(F.getName()) << "(";

      arguments.clear();
      returnStatements.clear();

      printInstructions(F);

      findReturnStatements(F, returnStatements);
      findArguments(F, arguments, returnStatements);

      ArgMap::iterator arg_i = arguments.begin();
      ArgMap::iterator arg_e = arguments.end();
      
      for(; arg_i != arg_e; ++arg_i) {
        Argument& arg = *arg_i->first;
        TaintSet l = arg_i->second;

        buildTaintSetFor(F, arg, l);
       
        RetMap::iterator ret_i = returnStatements.begin();
        RetMap::iterator ret_e = returnStatements.end();

        for (; ret_i != ret_e; ++ret_i) {
          Value& retval = *ret_i->first;
          TaintSet rets = ret_i->second;

          if (&retval == &arg) {
            debug() << "Skipping detected self-taint\n";
            continue;
          }

          debug() << "Ret-set for `" << retval << "`:\n",
          printSet(rets);
          debug() << "\n";
          TaintSet intersect;
          set_intersection(l.begin(), l.end(), rets.begin(), rets.end(), inserter(intersect, intersect.end()));

          if (intersect.begin() != intersect.end()) {
            release() << (isFirstTime ? "" : ", ") << arg.getName() << " => " << getValueNameOrDefault(retval);
            debug() << arg.getName() << " => " << getValueNameOrDefault(retval) << "\n";
            isFirstTime = false;
          }
        }
      }

      release() << ")\n";
      return false;
    }

    private:
    void buildTaintSetFor(Function& F, Argument& arg, TaintSet& taintSet) {
      for (inst_iterator inst_i = inst_begin(F), inst_e = inst_end(F); inst_i != inst_e; ++inst_i) {
        Instruction& inst = cast<Instruction>(*inst_i);
        debug() << "Inspecting instruction: " << inst << "\n";

        if (inst.getOpcode() == 2 && inst.getNumOperands() == 3)
          handleBranchInstruction(inst, taintSet);
        else
          handleInstruction(inst, taintSet);
      }

      debug() << "Taint set for arg `" << arg.getName() << "`:\n";
      printSet(taintSet);
      debug() << "\n";
    }

    void handleBranchInstruction(Instruction& inst, TaintSet& taintSet) {
      Instruction& cmp_inst = cast<Instruction>(*inst.getOperand(0));
      BasicBlock& brTrue = cast<BasicBlock>(*inst.getOperand(1));
      BasicBlock& brFalse = cast<BasicBlock>(*inst.getOperand(2));
      
      debug() << "  Inspecting branch instruction:\n";
      debug() << "    Cmp is: " << cmp_inst << "\n";
      debug() << "    true-block: " << brTrue << "\n";
      debug() << "    false-block: " << brFalse << "\n";

      if (taintSet.find(&cmp_inst) != taintSet.end()) {
        debug() << "    Condition seems tainted!\n";
        
      }
    }

    void handleInstruction(Instruction& inst, TaintSet& taintSet) {
      for (size_t o_i = 0; o_i < inst.getNumOperands(); o_i++) {
          debug() << "  Inspecting operand #" << o_i << "\n";
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
      if (!OUTPUT_RELEASE)
        return nulls();

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
        if (I.getOpcode() == Instruction::Store && I.getOperand(0) == &arg) {
          Value& op = cast<Value>(*I.getOperand(1));
          retlist.insert(&op);
          debug() << "Found store for `" << arg.getName() << "` @ " << op << "\n";

          findAllStoresAndLoadsForOutArgumentAndAddToSet(F, op, retlist);
        }
        
        if (I.getOpcode() == Instruction::Load && I.getOperand(0) == &arg) {
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
          if ((*i).getOpcode() == 1) {
            Value& r = cast<Value>(*i);
            //r.setName("ret_val");
            TaintSet l;
            l.insert(&r);
            retStmts.insert(pair<Value*, set<Value*> >(&r, l));
            debug() << "Found ret-stmt: " << r << "\n";
          }
      }
    }

    void printInstructions(Function &F) {
      debug() << "Instructions: \n";

      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        debug() << (*i) << "\n";
      }
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> X("dataflow", "Data-flow analysis", false, true);
