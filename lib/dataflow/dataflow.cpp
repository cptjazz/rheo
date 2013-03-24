#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CallGraph.h"
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
    map<Function*, Function*> _circularReferences;
    set<Function*> _queuedFunctionHelper;

    Dataflow() : ModulePass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<PerFunctionFlow>();
      AU.addRequired<CallGraph>();
    }

    void findCallGraphChildren(const CallGraphNode* node, const CallGraphNode* startNode) {
      DEBUG(errs() << "find children: " << node->getFunction()->getName() << " -- " << startNode->getFunction()->getName() << "\n");
      for (CallGraphNode::const_iterator i = node->begin(), e = node->end(); i != e; ++i) { 
        const CallGraphNode* kid = i->second;

        if (kid == startNode) {
          // Found circle.

          _circularReferences.insert(make_pair(node->getFunction(), startNode->getFunction()));
          continue;
        }

        if (kid->getFunction())
          findCallGraphChildren(kid, startNode);
      }

      Function* f = node->getFunction();
      addFunctionForProcessing(f);
    }

    void addFunctionForProcessing(Function* f) {
      if (!_queuedFunctionHelper.count(f)) {
        _functionQueue.push(f);
        _queuedFunctionHelper.insert(f);
      }
    }

    virtual bool runOnModule(Module &module) {
      CallGraph& CG = getAnalysis<CallGraph>();

      for (Module::iterator i = module.begin(), e = module.end(); i != e; ++i) {
        Function* f = &*i;

        // Skip if function was already processed.
        if (TaintFile::exists(*f))
          continue;

        const CallGraphNode* node = CG[f];

        findCallGraphChildren(node, node);
        addFunctionForProcessing(f);
        
        _occurrenceCount.insert(pair<Function*, int>(f, 1));
      }

      for (map<Function*, Function*>::iterator f_i = _circularReferences.begin(), f_e = _circularReferences.end(); f_i != f_e; ++f_i) {
        errs() << "Founc circ-ref: " << f_i->first->getName() << " <--> " << f_i->second->getName() << "\n";
      }

      while (!_functionQueue.empty()) {
        Function* f = _functionQueue.front();

        if (_occurrenceCount[f]++ > 3) {
          errs() << "__error:PANIC: detected endless loop. Aborting.\n";
          return false;
        }

        processFunction(*f);
        _functionQueue.pop();
      }

      return false;
    }

    void processFunction(Function& func) {
      // Skip external (library) functions
      if (!func.size())
        return;

      errs() << "# Run per function pass on `" << func.getName() << "`\n";
      errs() << "__log:start:" << func.getName() << "\n";

      PerFunctionFlow& pff = getAnalysis<PerFunctionFlow>(func);
      if (pff.getState() == Skip) {
        errs() << "Skipping `" << func.getName() << "`\n";
        return;
      }

      if (pff.getState() == Deferred) {
        errs() << "__log:defer:" << func.getName() << ": Deferring `" << func.getName() << "`\n";
        TaintFile::remove(func);
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

