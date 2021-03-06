#ifndef TAINTFILE_H
#define TAINTFILE_H

#include <string>
#include <map>
#include "Core.h"
#include "Helper.h"
#include "Logger.h"
#include "SpecialTaintHelper.h"


typedef std::map<const Function*, FunctionTaintMap> TaintFileCache;

class TaintFile {
public:
  static bool exists(const Function& f);
  static void writeTempResult(SpecialTaintHelper& sth, const Function& f, const ResultSet result);
  static void persistResult(const Function& f);
  static void remove(const Function& f);
  static const FunctionTaintMap* getMapping(const Function& func, const Logger& logger);

private:
  static TaintFileCache _mappingCache;

  static bool read(const Function& func, const Logger& logger, FunctionTaintMap& mapping);
  static std::string getFilename(const Function& f);
  static int getValuePosition(const Function& func, const Logger& logger, const std::string valName);
};

#endif // TAINTFILE_H
