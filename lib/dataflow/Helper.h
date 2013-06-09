#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <math.h>
#include "Core.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Argument.h"
#include "llvm/IR/GlobalVariable.h"
#include "Helper.h"

using namespace std;
using namespace llvm;

class Helper {
  public:
    /**
     * @return if the provided Value is either an Argument or a GlobalVariable, its name is returned. '$_retval' otherwise.
     */
    inline static string getValueNameOrDefault(const Value& v) {
      if (isa<Argument>(v) || isa<GlobalVariable>(v))
        return v.getName().str();
      else if (isa<ReturnInst>(v))
        return "$_retval";
      else 
        return "...";
    }

    /**
     * @return true if the function is in the provided circular reference set.
     */
    inline static bool circleListContains(NodeVector& v, const Function& f) {
      for (NodeVector::const_iterator i = v.begin(), e = v.end(); i != e; i++)
        if ((*i)->getFunction() == &f)
          return true;

      return false;
    }

    inline static long getTimestamp() {
      timespec t;
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
      return t.tv_nsec;
    }

    inline static long getTimestampDelta(long tOld) {
      long tNew = getTimestamp();
      return abs(tNew - tOld) * 0.001;
    }
};

#endif // HELPER_H
