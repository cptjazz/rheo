#include "llvm/Pass.h"
#include "llvm/PassManagers.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Support/Casting.h"
#include "llvm/InstrTypes.h"
#include <map>

using namespace llvm;
using namespace std;

namespace {

  struct LoopAnalyser : public LoopPass {
    static char ID;

    LoopAnalyser() : LoopPass(ID) { }

    virtual bool runOnLoop(Loop* L, LPPassManager& LPM) {
      errs() << "Found Loop at:";

      errs() << *L << "\n";
      return false;
    }
  };
}

char LoopAnalyser::ID = 0;
static RegisterPass<LoopAnalyser> X("loopanalyser", "Loop analysis", false, true);
