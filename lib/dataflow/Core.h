#ifndef CORE_H
#define CORE_H

#include <set>
#include <map>
#include <string>
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include "TaintSet.h"

//#define PROFILING

#ifdef PROFILING
  #define IF_PROFILING(x) x
#else
  #define IF_PROFILING(x)
#endif


using namespace std;
using namespace llvm;

struct FunctionTaint {
  FunctionTaint(string sourceName, int sourcePos, string sinkName, int sinkPos) {
    this->sourceName = sourceName;
    sourcePosition = sourcePos;
    this->sinkName = sinkName;
    sinkPosition = sinkPos;
  }

  FunctionTaint(int sourcePos, int sinkPos) {
    sourceName = "";
    sourcePosition = sourcePos;
    sinkName = "";
    sinkPosition = sinkPos;
  }

  bool operator<(const FunctionTaint& r) const {
    return this < &r;
  }

  bool operator==(const FunctionTaint& r) const {
    if (sourcePosition > -3 && r.sourcePosition > -3 && sinkPosition > -3 && r.sinkPosition > -3)
      return sourcePosition == r.sourcePosition && sinkPosition && r.sinkPosition;

    return false;
  }

  string sourceName;
  int sourcePosition;
  string sinkName;
  int sinkPosition;
};

typedef map<const Value*, TaintSet> TaintMap;
typedef pair<const Value*, const Value*> TaintPair;
typedef set<TaintPair> ResultSet;
typedef vector<const CallGraphNode*> NodeVector;
typedef map<const Function*, NodeVector> CircleMap;
typedef set<FunctionTaint> FunctionTaintMap;

#endif // CORE_H
