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
#include "TaintFile.h"

using namespace llvm;
using namespace std;

namespace {

  const unsigned int OUTPUT_RELEASE = 0;

  struct Dataflow : public ModulePass {
    static char ID;
    queue<Function*> _functionQueue;
    map<Function*, int> _occurrenceCount;
    FunctionMap _circularReferences;
    set<Function*> _queuedFunctionHelper;
    set<Function*> _avoidInfiniteLoopHelper;

    Dataflow() : ModulePass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<CallGraph>();
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

    void enqueueFunctionsInCorrectOrder(const CallGraphNode* node) {
      Function* f = node->getFunction();

      for (CallGraphNode::const_iterator i = node->begin(), e = node->end(); i != e; ++i) { 
        const CallGraphNode* kid = i->second;

        if (!kid->getFunction())
          continue;

        if (_avoidInfiniteLoopHelper.count(kid->getFunction())) {
          continue;
        }

        _avoidInfiniteLoopHelper.insert(kid->getFunction());
        enqueueFunctionsInCorrectOrder(kid);
      }

      // Skip external (library) functions
      if (f && f->size()) {
        addFunctionForProcessing(f);
      }
    }

    void buildCircularReferenceInfo(const CallGraphNode* node, const CallGraphNode* startNode) {
      for (CallGraphNode::const_iterator i = node->begin(), e = node->end(); i != e; ++i) { 
        const CallGraphNode* kid = i->second;
       
        if (kid == startNode) {
          // Found circle.
          _circularReferences.insert(make_pair(node->getFunction(), startNode->getFunction()));
          continue;
        }

        Function* f = kid->getFunction();
        if (_avoidInfiniteLoopHelper.count(f))
          // Already processed
          continue;

        _avoidInfiniteLoopHelper.insert(f);
        buildCircularReferenceInfo(kid, startNode);
      }
    }

    void addFunctionForProcessing(Function* f) {
      if (!_queuedFunctionHelper.count(f)) {
        _functionQueue.push(f);
        _queuedFunctionHelper.insert(f);
        DEBUG(errs() << "Enqueued: " << f->getName() << "\n");
      }
    }

    virtual bool runOnModule(Module &module) {
      CallGraph& CG = getAnalysis<CallGraph>();

      _queuedFunctionHelper.clear();
      _avoidInfiniteLoopHelper.clear();

      enqueueFunctionsInCorrectOrder(CG.getRoot());

      _avoidInfiniteLoopHelper.clear();
      for (Module::iterator i = module.begin(), e = module.end(); i != e; ++i) {
        Function* f = &*i;
        _occurrenceCount.insert(pair<Function*, int>(f, 1));
      }

      for (CallGraphNode::const_iterator i = CG.getRoot()->begin(), e = CG.getRoot()->end(); i != e; ++i) { 
        const CallGraphNode* kid = i->second;
        buildCircularReferenceInfo(kid, kid);
      }

      for (FunctionMap::const_iterator f_i = _circularReferences.begin(), f_e = _circularReferences.end(); f_i != f_e; ++f_i) {
        DEBUG(errs() << "Found circ-ref: " << f_i->first->getName() << " <--> " << f_i->second->getName() << "\n");
      }

      while (!_functionQueue.empty()) {
        Function* f = _functionQueue.front();

        // Skip if function was already processed.
        if (TaintFile::exists(*f)) {
          _functionQueue.pop();
          continue;
        }

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
      errs() << "# Run per function pass on `" << func.getName() << "`\n";
      errs() << "__log:start:" << func.getName() << "\n";

      ResultSet result;
      bool state = runOnFunction(func, result);

      if (!state) {
        errs() << "Skipping `" << func.getName() << "`\n";
        return;
      }

      TaintFile::writeResult(func, result);
    }
    
    bool runOnFunction(Function& func, ResultSet& result) {
      DominatorTree& dt = getAnalysis<DominatorTree>(func);
      PostDominatorTree& pdt = getAnalysis<PostDominatorTree>(func);

      long time = Helper::getTimestamp();

      FunctionProcessor proc(func, _circularReferences, *func.getParent(), dt, pdt, result, errs());
      proc.processFunction();
      bool finished = proc.didFinish();

      time = Helper::getTimestampDelta(time);
      
      errs() << "__logtime:" << func.getName() << ":" << time << " µs\n";

      return finished;
    }
  };
}

char Dataflow::ID = 0;

static RegisterPass<Dataflow> Y("dataflow", "Data-flow analysis", true, true);

