#ifndef CORE_H
#define CORE_H

#include <set>
#include <map>
#include <llvm/Value.h>

using namespace std;
using namespace llvm;

typedef set<Value*> TaintSet;
typedef map<Value*, TaintSet> TaintMap;
typedef pair<Value*, Value*> TaintPair;
typedef set<TaintPair> ResultSet;

#endif // CORE_H
