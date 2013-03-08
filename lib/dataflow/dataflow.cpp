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
#include <queue>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "GraphExporter.h"
#include "FunctionProcessor.h"
#include "PerFunctionFlow.cpp"
#include "TaintFile.h"

using namespace llvm;
using namespace std;

namespace {

  const unsigned int OUTPUT_RELEASE = 0;

  struct Dataflow : public ModulePass {
    static char ID;
    queue<Function*> _functionQueue;
    map<Function*, int> _occurrenceCount;

    Dataflow() : ModulePass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<PerFunctionFlow>();
    }

    virtual bool runOnModule(Module &module) {
      for (Module::iterator i = module.begin(), e = module.end(); i != e; ++i) {
        Function* f = &*i;
        _functionQueue.push(f);
        _occurrenceCount.insert(pair<Function*, int>(f, 1));
      }

      while (!_functionQueue.empty()) {
        Function* f = _functionQueue.front();

        if (_occurrenceCount[f]++ > 10) {
          errs() << "!!! PANIC: detected endless loop. Aborting.\n";
          return false;
        }

        processFunction(*f);
        _functionQueue.pop();
      }

      return false;
    }

    void processFunction(Function& func) {
      // Skip if function was already processed.
      if (TaintFile::exists(func))
        return;

      // Skip external (library) functions
      if (!func.size())
        return;

      errs() << "# Run per function pass on `" << func.getName() << "`\n";

      PerFunctionFlow& pff = getAnalysis<PerFunctionFlow>(func);
      if (pff.getState() == Skip) {
        errs() << "Skipping `" << func.getName() << "`\n";
        return;
      }

      if (pff.getState() == Deferred) {
        errs() << "Deferring `" << func.getName() << "`\n";
        _functionQueue.push(&func); 
        return;
      }

      ResultSet& result = pff.getResult();
      TaintFile::writeResult(func, result);
    }
  };
}

char Dataflow::ID = 0;

static RegisterPass<Dataflow> Y("dataflow", "Data-flow analysis", true, true);
static RegisterPass<PerFunctionFlow> X("per-function-flow", "Data-flow analysis for one function", true, true);

