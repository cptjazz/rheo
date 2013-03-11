#include "Helper.h"
#include <llvm/Argument.h>
#include <llvm/GlobalVariable.h>
#include <algorithm>
#include <math.h>

string Helper::getValueNameOrDefault(const Value& v) {
  if (isa<Argument>(v) || isa<GlobalVariable>(v))
    return v.getName().str();
  else
    return "$_retval";
}

bool Helper::setContains(const TaintSet& taintSet, const Value& val) {
   return taintSet.count(&val);
}
	 
void Helper::intersectSets(const TaintSet& argTaintSet, const TaintSet& retTaintSet, TaintSet& intersect) {
  for (TaintSet::const_iterator i = argTaintSet.begin(), e = argTaintSet.end(); i != e; ++i) {
    if (retTaintSet.count(*i))
      intersect.insert(*i);
  }
}
