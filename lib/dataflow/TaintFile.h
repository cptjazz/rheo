#ifndef TAINTFILE_H
#define TAINTFILE_H

#include <string>
#include <map>
#include <llvm/Function.h>
#include "llvm/Support/raw_ostream.h"
#include "Helper.h"

using namespace std;
using namespace llvm;


class TaintFile {
public:
  static bool exists(const Function& f);
  static void writeResult(const Function& f, const ResultSet result);
  static void remove(const Function& f);
  static bool getMapping(const Function& func, FunctionTaintMap& mapping, raw_ostream& debugStream);

private:
  static map<const Function*, FunctionTaintMap> _mappingCache;

  static bool read(const Function& func, raw_ostream& debugStream, FunctionTaintMap& mapping);
  static string getFilename(const Function& f);
};

#endif // TAINTFILE_H
