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
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "GraphExporter.h"
#include "FunctionProcessor.h"
#include "PerFunctionFlow.cpp"

using namespace llvm;
using namespace std;

namespace {

  const unsigned int OUTPUT_RELEASE = 0;

  struct Dataflow : public ModulePass {
    static char ID;

    Dataflow() : ModulePass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      //AU.addRequired<PerFunctionFlow>();
    }

    virtual bool runOnModule(Module &module) {
      for (Module::iterator i = module.begin(), e = module.end(); i != e; ++i) {
        Function& func = *i;

        // Skip if function was already processed.
        if (taintResultExists(func))
          continue;

        //PerFunctionFlow& pff = getAnalysis<PerFunctionFlow>(func);
        //ResultSet result = pff.getResult();

        //writeResult(func, result);
      }

      return false;
    }

    bool taintResultExists(Function& f) {
      ifstream file(f.getName().str().c_str());
      return file;
    }

    void writeResult(Function& f, ResultSet result) {
      ofstream file;
      file.open((f.getName().str() + ".taints").c_str(), ios::out);

      for (ResultSet::iterator i = result.begin(), e = result.end(); i != e; ++i) {
        Argument& arg = *i->first;
        Value& retval = *i->second;

        file << arg.getName().str() << " => ";
        if (isa<ReturnInst>(retval))
          file <<  "$_retval";
        else
	  file << retval.getName().str();

        file << "\n";
      }

      file.close();
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> Y("dataflow", "Data-flow analysis", true, true);
