#include "Helper.h"
#include <llvm/Argument.h>
#include <llvm/GlobalVariable.h>
#include <algorithm>

string Helper::getValueNameOrDefault(Value& v) {
  if (isa<Argument>(v) || isa<GlobalVariable>(v))
    return v.getName().str();
  else
    return "$_retval";
}

bool Helper::areSetsEqual(set<Value*>& s1, set<Value*>& s2) {
  set<Value*> diff;
  set_symmetric_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(diff, diff.begin()));

  return diff.size() == 0;
}
