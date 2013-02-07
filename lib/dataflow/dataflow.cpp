#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/InstrTypes.h"
#include <map>
#include <set>
#include <stdio.h>


using namespace llvm;
using namespace std;

namespace {

  const unsigned int OPCODE_RET = 1;

  struct Dataflow : public FunctionPass {
    static char ID;

    set<Instruction*> returnStatements;
    map<Argument*, Value*> arguments;

    Dataflow() : FunctionPass(ID) { }

    virtual bool runOnFunction(Function &F) {
      errs() << "\n";
      errs().write_escaped(F.getName()) << "\n";
      errs() << "========================================\n";

      arguments.clear();
      returnStatements.clear();

      printInstructions(F);

      findReturnStatements(F, returnStatements);
      printUseDefForReturnStatements(returnStatements);

      findArguments(F, arguments);
      printDefUseForArguments(arguments);

      for (set<Instruction*>::iterator ret_i = returnStatements.begin(), ret_e = returnStatements.end(); ret_i != ret_e; ret_i++) {
        User& returnStmt = *cast<User>(*ret_i);
        iterateOverUseDefChain(returnStmt);
      }

      return false;
    }

    private:
    bool iterateOverUseDefChain(User& chainHead) {
      if (searchInArgumentChain(chainHead, arguments))
        return true;

      for (User::op_iterator i = chainHead.op_begin(), e = chainHead.op_end(); i != e; ++i) {
        User& u = *cast<User>(*i);
        if (iterateOverUseDefChain(u))
          return true;
      }

      return false;
    }

    bool searchInArgumentChain(Value& ret_val, map<Argument*, Value*>& arguments) {
      for (map<Argument*, Value*>::iterator arg_i = arguments.begin(), arg_e = arguments.end(); arg_i != arg_e; arg_i++) {
        Value& head = *arg_i->first;
        if (isValueInDefUseChain(ret_val, *arg_i->second)) {
          errs() << "Argument `" << head.getName() << "` taints return value `" << ret_val << "`";
          return true;
        }
      }

      return false;
    }

    void findArguments(Function& F, map<Argument*, Value*>& args) {
      for (Function::arg_iterator i = F.arg_begin(), e = F.arg_end(); i != e; ++i) {
        Argument& arg = *i;
        Value& arg_tree_start = getBaseUserForArgument(arg);
        args.insert(pair<Argument*, Value*>(&arg, &arg_tree_start));
      }
    }

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

    void printDefUseForArguments(map<Argument*, Value*>& args) {
      for (map<Argument*, Value*>::iterator i = args.begin(), e = args.end(); i != e; ++i) {
        Value& storeLoc = *i->second;
        Argument& arg = *i->first;
        errs() << "Found argument: " << arg.getName() << "\n";
        printUsages(storeLoc, 0);
      }
    }

    bool isValueInDefUseChain(Value& v, Value& chainHead) {
      errs() << "head: " << chainHead << "  v: " << v << "\n";
      if (&chainHead == &v) {
	errs() << "match: " << chainHead << " -> " << v << "\n";
        return true;
	}
      
      for (Value::use_iterator i = chainHead.use_begin(), e = chainHead.use_end(); i != e; ++i) {
        User *u = *i;
        Value* next = cast<Value>(u);
        if (isValueInDefUseChain(v, *next))
          return true;
      }

      return false;
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
