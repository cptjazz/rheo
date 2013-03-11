#ifndef HELPER_H
#define HELPER_H

#include <string>
#include "Core.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace llvm;

class Helper {
public:
  static string getValueNameOrDefault(const Value& v);   
  static bool setContains(const TaintSet& taintSet, const Value& val);
  static void intersectSets(const TaintSet& argTaintSet, const TaintSet& retTaintSet, TaintSet& intersect);

  inline
    static long getTimestamp() {
      timespec t;
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
      return t.tv_nsec;
    }

  inline
    static long getTimestampDelta(long tOld) {
      long tNew = getTimestamp();
      return abs(tNew - tOld) * 0.001;
    }

};

#endif // HELPER_H
