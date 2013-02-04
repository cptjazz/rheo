#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/InstrTypes.h"


using namespace llvm;

namespace {

  struct Dataflow : public FunctionPass {
    static char ID;

    Dataflow() : FunctionPass(ID) { }

    virtual bool runOnFunction(Function &F) {
      errs() << "\n";
      errs().write_escaped(F.getName()) << "\n";
      errs() << "========================================\n";


      errs() << "Arg infos: \n";
      Function::arg_iterator a_i = F.arg_begin();
      Function::arg_iterator a_e = F.arg_end();
      for (; a_i != a_e; ++a_i) {
        Argument& arg = *a_i;
        errs() << arg.getName() << ":\n";
        Value& arg_tree_start = getBaseUserForArgument(arg);
        printUsages(arg_tree_start, 0);
      }

      return false;
    }

    private:
    Value& getBaseUserForArgument(Value& val) {
      // assume val is an argument and always has one user.
      User* user = cast<User>(*(val.use_begin()));
      Instruction* inst = cast<Instruction>(user);
      return *(inst->getOperand(1)); 
    }

    void printUsages(Value &V, int level) {
      for (int j = 0; j < level; j++)
        errs() << " ";

      errs() << V << "\n";

      Value::use_iterator i = V.use_begin();
      Value::use_iterator e = V.use_end();
      
      for (; i != e; ++i) {
        Value& v = *cast<Value>(*i);
        printUsages(v, level + 1);
      }
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> X("dataflow", "Data-flow analysis", false, true);
