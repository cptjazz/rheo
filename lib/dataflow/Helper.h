#ifndef HELPER_H
#define HELPER_H

#include <string>
#include "Core.h"

using namespace std;
using namespace llvm;

class Helper {
public:
  static string getValueNameOrDefault(Value& v);   
  static bool areSetsEqual(TaintSet& s1, TaintSet& s2);
  static bool setContains(TaintSet& taintSet, Value& val);
};

#endif // HELPER_H
