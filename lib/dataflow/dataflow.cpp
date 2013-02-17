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
#include <stdio.h>
#include "GraphExporter.h"
#include "FunctionProcessor.h"


using namespace llvm;
using namespace std;

typedef set<Value*> TaintSet;
typedef map<Value*, TaintSet> RetMap;
typedef map<Argument*, TaintSet> ArgMap;
typedef pair<Argument*, Value*> TaintPair;
typedef set<TaintPair> ResultSet;

namespace {

  const unsigned int OUTPUT_RELEASE = 0;

  struct Dataflow : public ModulePass {
    static char ID;

    Dataflow() : ModulePass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

    virtual bool runOnModule(Module &module) {
      for (Module::iterator i = module.begin(), e = module.end(); i != e; ++i) {
        Function& func = *i;
        errs() << "## Running analysis for `" << func.getName() << "`\n";

        DominatorTree& dt = getAnalysis<DominatorTree>(func);
        PostDominatorTree& pdt = getAnalysis<PostDominatorTree>(func);
        FunctionProcessor proc(func, dt, pdt, errs());
        proc.processFunction();
      }

      return false;
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> X("dataflow", "Data-flow analysis", true, true);
