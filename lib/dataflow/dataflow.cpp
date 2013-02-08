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
#include <stdio.h>


using namespace llvm;
using namespace std;

namespace {

  const unsigned int OPCODE_RET = 1;

  struct Dataflow : public FunctionPass {
    static char ID;

    set<Value*> returnStatements;
    map<Argument*, set<Value*> > arguments;

    Dataflow() : FunctionPass(ID) { }

    virtual bool runOnFunction(Function &F) {
      errs() << "\nInspecting function `";
      errs().write_escaped(F.getName()) << "`\n";
      errs() << "- - - - - - - - - - - - - - - - \n";

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
          //errs() << "Inspecting instruction: " << *inst_i << "\n";
      	  for (size_t o_i = 0; o_i < inst_i->getNumOperands(); o_i++) {
            //errs() << "  Inspecting operand #" << o_i << "\n";
            Value& operand = *inst_i->getOperand(o_i);
            if (l.find(&operand) != l.end()) {
              addValueToSet(l, *inst_i);
              //errs() << "    Added " << *&*inst_i << "\n";
            }
          }
        }

        errs() << "Taint set for arg `" << arg.getName() << "`:\n";
        printSet(l);
        errs() << "\n";
        set<Value*> intersect;
        set_intersection(l.begin(), l.end(), returnStatements.begin(), returnStatements.end(), 
          inserter(intersect, intersect.end()));

        if (intersect.begin() != intersect.end())
          errs() << arg.getName() << " taints return value!\n";
      }


      return false;
    }

    private:
    void addValueToSet(set<Value*>& l, Instruction& I) {
      if (I.getOpcode() == Instruction::Store)
        l.insert(I.getOperand(1));
      else
        l.insert(&I);
    }

    void findArguments(Function& F, map<Argument*, set<Value*> >& args, set<Value*>& retStmts) {
      for (Function::arg_iterator i = F.arg_begin(), e = F.arg_end(); i != e; ++i) {
        Argument& arg = *i;
        set<Value*> l;
        if (! arg.getType()->isPointerTy()) {
          l.insert(&arg);
          args.insert(pair<Argument*, set<Value*> >(&arg, l));
errs() << "added arg `" << arg.getName() << "` to arg-list\n";
        } else {
          findAllStoresAndLoadsForOutArgumentAndAddToSet(arg, retStmts, F);
errs() << "added arg `" << arg.getName() << "` to out-list\n";
        }
      }
    }

    void findAllStoresAndLoadsForOutArgumentAndAddToSet(Value& arg, set<Value*>& retStmts, Function& F) {
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        Instruction& I = cast<Instruction>(*i);
errs() << "inspecting: " << I << "\n";
        if (I.getOpcode() == Instruction::Store && I.getOperand(0) == &arg) {
          Value& op = cast<Value>(*I.getOperand(1));
          retStmts.insert(&op);
          errs() << "Found store for `" << arg.getName() << "` @ " << op << "\n";

          findAllStoresAndLoadsForOutArgumentAndAddToSet(op, retStmts, F);
        }
        
        if (I.getOpcode() == Instruction::Load && I.getOperand(0) == &arg) {
          Value& op = cast<Value>(I);
          retStmts.insert(&op);
          errs() << "Found load for `" << arg.getName() << "` @ " << op << "\n";
          
          findAllStoresAndLoadsForOutArgumentAndAddToSet(op, retStmts, F);
        }
      }
    }

    void printSet(set<Value*>& s) {
      for (set<Value*>::iterator i = s.begin(), e = s.end(); i != e; ++i) {
        errs() << **i << " | ";
      }
    } 
    void findReturnStatements(Function& F, set<Value*>& retStmts) {
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
          if ((*i).getOpcode() == 1) {
            Value& r = cast<Value>(*i);
            //r.setName("ret_val");
            retStmts.insert(cast<Value>(&r));
            errs() << "Found ret-stmt: " << r << "\n";
          }
      }
    }

    void printInstructions(Function &F) {
      errs() << "Instructions: \n";

      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        errs() << (*i) << "\n";
      }
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> X("dataflow", "Data-flow analysis", false, true);
