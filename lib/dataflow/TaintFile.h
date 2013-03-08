#ifndef TAINTFILE_H
#define TAINTFILE_H

#include <string>
#include <map>
#include <llvm/Function.h>
#include "llvm/Support/raw_ostream.h"
#include "Helper.h"

using namespace std;
using namespace llvm;

typedef map<int, int> FunctionTaintMap;

class TaintFile {
public:
  static TaintFile* read(Function& func, raw_ostream& debugStream);
  static bool exists(Function& f);
  static void writeResult(Function& f, ResultSet result);
  static void remove(Function& f);
  FunctionTaintMap& getMapping() { return _functionTaintMap; }

private:
  FunctionTaintMap _functionTaintMap;

  static string getFilename(Function& f);
};

#endif // TAINTFILE_H
