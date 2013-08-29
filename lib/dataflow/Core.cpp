#include "Core.h"
#include "llvm/Support/CommandLine.h"

cl::opt<bool> Configuration::CreateGraph("graph", cl::desc("Create graphical representation of information flow"));

cl::opt<std::string> Configuration::RequestFilePath(
    "request", 
    cl::desc("A file which contains information about which slice of the BC file is to be analysed."),
    cl::init(""));

cl::opt<std::string> Configuration::ExcludeFilePath(
    "exclude", 
    cl::desc("A file which specifies what functions are forced to be approximated by the heuristic, "
      "even if the functions are non-external. Useful to work around functions with not supported instructions."), 
    cl::init(""));
