#ifndef CORE_H
#define CORE_H

#define DEBUG_TYPE "dataflow"

#include "TaintSet.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <set>
#include <string>


// Inspired by LLVM's DEBUG macro in llvm/Support/Debug.h
extern bool GraphFlag;
#define IF_GRAPH(x) do { if (GraphFlag) { x; } } while(0)


//#define PROFILING
#ifdef PROFILING
  #define IF_PROFILING(x) x
#else
  #define IF_PROFILING(x)
#endif


using namespace llvm;

struct FunctionTaint {
  FunctionTaint(std::string sourceName, int sourcePos, std::string sinkName, int sinkPos) {
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

  std::string sourceName;
  int sourcePosition;
  std::string sinkName;
  int sinkPosition;
};

enum TaintType {
  NoTaint = 0,
  Source = 1,
  Sink = 2
};

inline TaintType operator|(TaintType a, TaintType b) {
  return static_cast<TaintType>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(TaintType a, TaintType b) {
  return (static_cast<int>(a) & static_cast<int>(b));
}

typedef SmallPtrSet<const Value*, 16> ValueSet;
typedef std::map<const Value*, TaintSet> TaintMap;
typedef std::pair<const Value*, const Value*> TaintPair;
typedef std::set<TaintPair> ResultSet;
typedef std::vector<const CallGraphNode*> NodeVector;
typedef std::map<const Function*, NodeVector> CircleMap;
typedef std::set<FunctionTaint> FunctionTaintMap;

#define RETURN_POSITION -1
#define VARARG_POSITION -2
#define GLOBAL_POSITION -3

#define RETURN_POSITION_STR "-1"
#define VARARG_POSITION_STR "-2"
#define GLOBAL_POSITION_STR "-3"

#endif // CORE_H
