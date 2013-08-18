#include "llvm/Support/CommandLine.h"
#include <string>

struct Configuration {
  static llvm::cl::opt<bool> CreateGraph;
  static llvm::cl::opt<std::string> ExcludeFilePath;
  static llvm::cl::opt<std::string> RequestFilePath;

  // Initialisation of static members see Core.cpp
};
