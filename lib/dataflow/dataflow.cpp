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

namespace {

  const unsigned int OUTPUT_RELEASE = 1;

  struct Dataflow : public FunctionPass {
    static char ID;

    map<Value*, set<Value*> > returnStatements;
    map<Argument*, set<Value*> > arguments;

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

      map<Argument*, set<Value*> >::iterator arg_i = arguments.begin();
      map<Argument*, set<Value*> >::iterator arg_e = arguments.end();
      
      for(; arg_i != arg_e; ++arg_i) {
        Argument& arg = *arg_i->first;
        set<Value*> l = arg_i->second;

        for (inst_iterator inst_i = inst_begin(F), inst_e = inst_end(F); inst_i != inst_e; ++inst_i) {
          debug() << "Inspecting instruction: " << *inst_i << "\n";
      	  for (size_t o_i = 0; o_i < inst_i->getNumOperands(); o_i++) {
            debug() << "  Inspecting operand #" << o_i << "\n";
            Value& operand = *inst_i->getOperand(o_i);
            if (l.find(&operand) != l.end()) {
              addValueToSet(l, *inst_i);
              debug() << "    Added " << *&*inst_i << "\n";
            }
          }
        }

        debug() << "Taint set for arg `" << arg.getName() << "`:\n";
        printSet(l);
        debug() << "\n";
        
        map<Value*, set<Value*> >::iterator ret_i = returnStatements.begin();
        map<Value*, set<Value*> >::iterator ret_e = returnStatements.end();

        for (; ret_i != ret_e; ++ret_i) {
          Value& retval = *ret_i->first;
          set<Value*> rets = ret_i->second;
          debug() << "Ret-set for `" << retval << "`:\n",
          printSet(rets);
          debug() << "\n";
          set<Value*> intersect;
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

    void findArguments(Function& F, map<Argument*, set<Value*> >& args, map<Value*, set<Value*> >& retStmts) {
      for (Function::arg_iterator i = F.arg_begin(), e = F.arg_end(); i != e; ++i) {
        Argument& arg = *i;
        set<Value*> l;
        if (! arg.getType()->isPointerTy()) {
          l.insert(&arg);
          args.insert(pair<Argument*, set<Value*> >(&arg, l));
          debug() << "added arg `" << arg.getName() << "` to arg-list\n";
        } else {
          set<Value*> retlist;
          findAllStoresAndLoadsForOutArgumentAndAddToSet(F, arg, retlist);
          retStmts.insert(pair<Value*, set<Value*> >(&arg, retlist));
          debug() << "added arg `" << arg.getName() << "` to out-list\n";
        }
      }
    }

    void findAllStoresAndLoadsForOutArgumentAndAddToSet(Function& F, Value& arg, set<Value*>& retlist) {
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
      for (set<Value*>::iterator i = s.begin(), e = s.end(); i != e; ++i) {
        debug() << **i << " | ";
      }
    } 
    void findReturnStatements(Function& F, map<Value*, set<Value*> >& retStmts) {
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
          if ((*i).getOpcode() == 1) {
            Value& r = cast<Value>(*i);
            //r.setName("ret_val");
            set<Value*> l;
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
