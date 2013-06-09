#include "Core.h"

using namespace llvm;

class IntrinsicHelper {

public:
  static bool getMapping(const Function& f, FunctionTaintMap& mapping);
};
