#include "Core.h"

using namespace llvm;

class IntrinsicHelper {

public:
  static bool getMapping(const Function& f, FunctionTaintMap& mapping);
  static bool shouldIgnoreCall(const Function& f);
};
