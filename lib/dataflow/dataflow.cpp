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


F.viewCFG();

      errs() << "Arg infos: \n";
      Function::arg_iterator a_i = F.arg_begin();
      Function::arg_iterator a_e = F.arg_end();
      for (; a_i != a_e; ++a_i) {
        Argument& arg = *a_i;
        Value& v = cast<Value>(arg);
        errs() << v.getName() << ": ";
        printUsages(v, 0);
        errs() << "\n";
      }

      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        errs() << (i->getParent()) << " " << *i << " || ";
        printUsages(*i, 0);
        errs() << "\n";
        handleInstructions(*i);
      }

      return false;
    }

    private:
    void handleInstructions(Instruction &I) {
      handleBinaryOperator(I);

      Value& val = cast<Value>(I);
    }

    void handleBinaryOperator(Instruction &I) {
      if (!isa<BinaryOperator>(I))
        return;

      BinaryOperator& bin_op = cast<BinaryOperator>(I);
      errs() << "     prev opcode: " << bin_op.getOpcode() << "\n";
      User& user = cast<User>(I);
      errs() << " num of operands: " << user.getNumOperands() << "\n";
      /*
      for (size_t i = 0; i < user.getNumOperands(); i++) {
        Value& v = *user.getOperand(i);
        printValueInfo(v);
        printUsages(v, 0);
      }*/

    }

    void printUsages(Value &V, int level) {
      Value::use_iterator i = V.use_begin();
      Value::use_iterator e = V.use_end();
      
      for (int j = 0; j < level; j++)
        errs() << "`";

      for (; i != e; ++i) {
        Value& v = *cast<Value>(*i);

        printValueInfo(v);
        printUsages(v, level + 1);
      }
    }

    void printValueInfo(Value &V) {
      errs() << "(" << V.getName() << "," << V << ")" << "/" 
                                      << V.getNumUses();
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> X("dataflow", "Data-flow analysis", false, true);
