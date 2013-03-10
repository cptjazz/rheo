#include "Helper.h"
#include <llvm/Argument.h>
#include <llvm/GlobalVariable.h>
#include <algorithm>
#include <math.h>

string Helper::getValueNameOrDefault(Value& v) {
  if (isa<Argument>(v) || isa<GlobalVariable>(v))
    return v.getName().str();
  else
    return "$_retval";
}

bool Helper::areSetsEqual(TaintSet& s1, TaintSet& s2) {
  long t = getTimestamp();

  set<Value*> diff;
  set_symmetric_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(diff, diff.begin()));

  return diff.size() == 0;
}

bool Helper::setContains(TaintSet& taintSet, Value& val) {
   return taintSet.find(&val) != taintSet.end();
}
	 
long Helper::getTimestamp() {
  timespec t;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
  return t.tv_nsec;
}

long Helper::getTimestampDelta(long tOld) {
  long tNew = getTimestamp();
  return abs(tNew - tOld) * 0.001;
}
