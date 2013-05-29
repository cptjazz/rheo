#ifndef TAINTFILE_H
#define TAINTFILE_H

#include <string>
#include <map>
#include <llvm/Function.h>
#include "llvm/Support/raw_ostream.h"
#include "Helper.h"
#include "Logger.h"

using namespace std;
using namespace llvm;

#define TaintFileCache map<const Function*, FunctionTaintMap>

class TaintFile {
public:
  static bool exists(const Function& f);
  static void writeResult(const Function& f, const ResultSet result);
  static void remove(const Function& f);
  static const FunctionTaintMap* getMapping(const Function& func, const Logger& logger);

private:
  static TaintFileCache _mappingCache;

  static bool read(const Function& func, const Logger& logger, FunctionTaintMap& mapping);
  static string getFilename(const Function& f);
};

#endif // TAINTFILE_H
