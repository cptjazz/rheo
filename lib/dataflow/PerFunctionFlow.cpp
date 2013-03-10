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
#include <time.h>
#include "GraphExporter.h"
#include "FunctionProcessor.h"


using namespace llvm;
using namespace std;

enum PerFunctionResultState {
  Success,
  Deferred,
  Skip
};

namespace {
  struct PerFunctionFlow : public FunctionPass {
    static char ID;
    ResultSet _result;
    bool _finished;
    PerFunctionResultState _state;

    PerFunctionFlow() : FunctionPass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

    virtual bool runOnFunction(Function& func) {
      _result = ResultSet();
      errs() << "## Running analysis for `" << func.getName() << "`\n";

      DominatorTree* dt = getAnalysisIfAvailable<DominatorTree>();
      PostDominatorTree* pdt = getAnalysisIfAvailable<PostDominatorTree>();

      if (!dt || !pdt) {
        errs() << "Skipping `" << func.getName() << "`\n";
        _state = Skip;
	      return false;
      }
      
      long time = Helper::getTimestamp();

      FunctionProcessor proc(func, *func.getParent(), *dt, *pdt, _result, errs());
      proc.processFunction();
      _finished = proc.didFinish();

      time = Helper::getTimestampDelta(time);
      
      errs() << "__logtime:" << func.getName() << ":" << time << " µs\n";

      _state = _finished ? Success : Deferred;
      return false;
    }

    ResultSet& getResult() {
      return _result;
    }

    PerFunctionResultState getState() {
      return _state;
    }

    bool didFinish() {
      return _finished;
    }
  };
}

char PerFunctionFlow::ID = 0;

#endif // PER_FUNCTION_PASS_H
