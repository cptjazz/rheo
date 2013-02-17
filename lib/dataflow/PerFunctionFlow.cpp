#ifndef PER_FUNCTION_PASS_H
#define PER_FUNCTION_PASS_H

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
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
#include "GraphExporter.h"
#include "FunctionProcessor.h"


using namespace llvm;
using namespace std;

namespace {
  struct PerFunctionFlow : public FunctionPass {
    static char ID;
    ResultSet _result;
    bool finished;

    PerFunctionFlow() : FunctionPass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

    virtual bool runOnFunction(Function& func) { return runOnFunctionImpl(func); }

    bool runOnFunctionImpl(Function& func) {
      _result = ResultSet();
      errs() << "## Running analysis for `" << func.getName() << "`\n";

      DominatorTree& dt = getAnalysis<DominatorTree>();
      PostDominatorTree& pdt = getAnalysis<PostDominatorTree>();

      FunctionProcessor proc(func, dt, pdt, errs());
      proc.processFunction(_result);
      finished = proc.didFinish();

      return false;
    }

    ResultSet& getResult() {
      return _result;
    }

    bool didFinish() {
      return finished;
    }
  };
}

#ifndef PFF_REG_PASS
#define PFF_REG_PASS
char PerFunctionFlow::ID = 0;
static RegisterPass<PerFunctionFlow> X("per-function-flow", "Data-flow analysis for one function", true, true);
#endif // PFF_REG_PASS

#endif // PER_FUNCTION_PASS_H
