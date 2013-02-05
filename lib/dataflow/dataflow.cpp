#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/InstrTypes.h"
#include <map>
#include <set>

using namespace llvm;
using namespace std;

namespace {

  const unsigned int OPCODE_RET = 1;

  struct Dataflow : public FunctionPass {
    static char ID;

    map<Argument*, Value*> argumentTreeHeads;

    Dataflow() : FunctionPass(ID) { }

    virtual bool runOnFunction(Function &F) {
      errs() << "\n";
      errs().write_escaped(F.getName()) << "\n";
      errs() << "========================================\n";

      printInstructions(F);
      printDefUseForArguments(F);

      set<Instruction*> returnStatements;
      findReturnStatements(F, returnStatements);
      printUseDefForReturnStatements(returnStatements);

      return false;
    }

    private:
    void printUseDefForReturnStatements(set<Instruction*>& retStmts) {
      for (set<Instruction*>::iterator i = retStmts.begin(), e = retStmts.end(); i != e; ++i) {
        User& u = cast<User>(**i);
        printOps(u, 0);
      }
    }

    void findReturnStatements(Function& F, set<Instruction*>& retStmts) {
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
          if ((*i).getOpcode() == 1) {
            retStmts.insert(&*i);
            errs() << "Fount ret-stmt: " << *i << "\n";
          }
      }
    }

    void printInstructions(Function &F) {
      errs() << "Instructions: \n";

      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        errs() << (*i) << "\n";
      }
    }

    void printDefUseForArguments(Function &F) {
      errs() << "Arg infos: \n";
      Function::arg_iterator a_i = F.arg_begin();
      Function::arg_iterator a_e = F.arg_end();
      for (; a_i != a_e; ++a_i) {
        Argument& arg = *a_i;
        errs() << arg.getName() << ":\n";
        Value& arg_tree_start = getBaseUserForArgument(arg);
        argumentTreeHeads.insert(pair<Argument*, Value*>(&arg, &arg_tree_start));
        printUsages(arg_tree_start, 0);
      }
    }

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

      for (Value::use_iterator i = V.use_begin(), e = V.use_end(); i != e; ++i) {
        Value& v = *cast<Value>(*i);
        printUsages(v, level + 1);
      }
    }

    void printOps(User &U, int level) {
      for (int j = 0; j < level; j++)
        errs() << " ";

      errs() << U << "\n";

      for (User::op_iterator i = U.op_begin(), e = U.op_end(); i != e; ++i) {
        User& u = *cast<User>(*i);
        printOps(u, level + 1);
      }
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> X("dataflow", "Data-flow analysis", false, true);
