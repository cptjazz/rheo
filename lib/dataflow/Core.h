#ifndef CORE_H
#define CORE_H

#include <set>
#include <map>
#include <llvm/Value.h>
#include <llvm/ADT/SmallPtrSet.h>
#include "llvm/Support/Debug.h"
#include "llvm/Analysis/CallGraph.h"
#include "TaintSet.h"

//#define PROFILING

#ifdef PROFILING
  #define IF_PROFILING(x) x
#else
  #define IF_PROFILING(x)
#endif


using namespace std;
using namespace llvm;

typedef map<const Value*, TaintSet> TaintMap;
typedef pair<const Value*, const Value*> TaintPair;
typedef set<TaintPair> ResultSet;
typedef vector<const CallGraphNode*> NodeVector;
typedef map<const Function*, NodeVector> CircleMap;
typedef set<pair<int, int> > FunctionTaintMap;

#endif // CORE_H
