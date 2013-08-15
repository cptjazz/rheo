#include "Core.h"
#include "llvm/Support/CommandLine.h"

bool GraphFlag;

cl::opt<bool, true> Graph("graph", cl::desc("Create graphical representation of information flow"), 
    cl::location(GraphFlag));
