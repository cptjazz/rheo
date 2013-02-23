#ifndef FUNCTION_PROCESSOR_H
#define FUNCTION_PROCESSOR_H

#include "llvm/Pass.h"
#include "llvm/Function.h"
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


using namespace llvm;
using namespace std;

typedef set<Value*> TaintSet;
typedef map<Value*, TaintSet> RetMap;
typedef map<Argument*, TaintSet> ArgMap;
typedef pair<Argument*, Value*> TaintPair;
typedef set<TaintPair> ResultSet;


class FunctionProcessor {
  Function& F;
  DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter* DOT;

  RetMap _returnStatements;
  ArgMap _arguments;
  ResultSet& _taints;

  raw_ostream& _stream;

  bool canceledInspection;

public:
  FunctionProcessor(Function& f, DominatorTree& dt, PostDominatorTree& pdt, ResultSet& result, raw_ostream& stream) 
  : F(f), DT(dt), PDT(pdt), _taints(result), _stream(stream)  { 
    DOT = new GraphExporter(f.getName());
    canceledInspection = false;
  }

  ~FunctionProcessor() {
    delete(DOT);
  }

  void processFunction();
  bool didFinish();

private:
  void intersectSets(Argument& arg, TaintSet argTaintSet);
  void buildTaintSetFor(Value& arg, TaintSet& taintSet);
  void addTaint(Argument& tainter, Value& taintee);
  bool setContains(TaintSet& taintSet, Value& val);
  void processBasicBlock(BasicBlock& block, TaintSet& taintSet);
  void printTaints();
  void handleGetElementPtrInstruction(GetElementPtrInst& storeInst, TaintSet& taintSet);
  void handleStoreInstruction(StoreInst& storeInst, TaintSet& taintSet);
  void handleCallInstruction(CallInst& callInst, TaintSet& taintSet);
  void handleBranchInstruction(BranchInst& inst, TaintSet& taintSet);
  void handleSwitchInstruction(SwitchInst& inst, TaintSet& taintSet);
  void handleInstruction(Instruction& inst, TaintSet& taintSet);
  bool handleBlockTainting(TaintSet& taintSet, Instruction& inst);
  StringRef getValueNameOrDefault(Value& v);
  void findArguments();
  void findAllStoresAndLoadsForOutArgumentAndAddToSet(Value& arg, TaintSet& retlist);
  void printSet(set<Value*>& s);
  void findReturnStatements();
  void printInstructions(); 
  void readTaintsFromFile(TaintSet& taintSet, CallInst& callInst, Function& func, ResultSet& result);

  raw_ostream& debug() {
    return _stream;
  }

  raw_ostream& release() {
    return _stream;
  }
};

#endif // FUNCTION_PROCESSOR_H
