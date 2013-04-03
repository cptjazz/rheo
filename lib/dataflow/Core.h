#ifndef CORE_H
#define CORE_H

#include <set>
#include <map>
#include <llvm/Value.h>
#include <llvm/ADT/SmallPtrSet.h>
#include "llvm/Support/Debug.h"

using namespace std;
using namespace llvm;

typedef SmallPtrSet<const Value*, 256> TaintSet;
typedef SmallPtrSet<const Value*, 256> ReturnSet;
typedef map<const Value*, TaintSet> TaintMap;
typedef pair<const Value*, const Value*> TaintPair;
typedef set<TaintPair> ResultSet;
typedef set<pair<const Function*, const Function*> > FunctionMap;
typedef map<int, int> FunctionTaintMap;

#endif // CORE_H
