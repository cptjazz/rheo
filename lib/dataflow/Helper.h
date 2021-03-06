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


class Helper {
  public:
    /**
     * @return if the provided Value is either an Argument, a special taint or 
     * a GlobalVariable, its name is returned. '$_retval' otherwise.
     */
    static std::string getValueName(const Value& v) {
      if (isa<Argument>(v))
        return v.getName();
      else if (isa<GlobalVariable>(v))
        return ("@" + v.getName()).str();
      else if (isa<ReturnInst>(v))
        return "$_retval";
      else if (v.getName().startswith("+"))
        return v.getName();
      else 
        return "...";
    }

    /**
     * @return true if the function is in the provided circular reference set.
     */
    static bool circleListContains(NodeVector& v, const Function& f) {
      for (auto i : v)
        if (i->getFunction() == &f)
          return true;

      return false;
    }

    static long getTimestamp() {
      timespec t;
      clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
      return t.tv_nsec;
    }

    static long getTimestampDelta(long tOld) {
      long tNew = getTimestamp();
      return abs(tNew - tOld) * 0.001;
    }
};

#endif // HELPER_H
