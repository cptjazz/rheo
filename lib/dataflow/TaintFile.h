#ifndef TAINTFILE_H
#define TAINTFILE_H

#include <string>
#include <map>
#include "Core.h"
#include "Helper.h"
#include "Logger.h"

using namespace std;
using namespace llvm;

typedef map<const Function*, FunctionTaintMap> TaintFileCache;

class TaintFile {
public:
  static bool exists(const Function& f);
  static void writeTempResult(const Function& f, const ResultSet result);
  static void persistResult(const Function& f);
  static void remove(const Function& f);
  static const FunctionTaintMap* getMapping(const Function& func, const Logger& logger);

private:
  static TaintFileCache _mappingCache;

  static bool read(const Function& func, const Logger& logger, FunctionTaintMap& mapping);
  static string getFilename(const Function& f);
};

#endif // TAINTFILE_H
