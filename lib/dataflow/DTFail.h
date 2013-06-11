#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CallGraph.h"

using namespace llvm;
using namespace std;


class DTFail : public ModulePass {

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

    bool runOnModule(Module &module); 

public:
    static char ID;

    DTFail() : ModulePass(ID) {
      errs() << "Hello from DomTree Fail\n";
    }
  };
