#ifndef EXCLUDE_FILE_H
#define EXCLUDE_FILE_H

#include "Core.h"
#include <string>
#include <fstream>

class ExcludeFile {
  private:
    std::set<std::string> functions;
    bool hasFile;

    ExcludeFile() { 
      hasFile = false;
    }

  public:
    inline bool includesFunction(const Function* function) {
      // Default: no exclusions
      if (!hasFile)
        return false;

      // This handles the virtual root node
      // of the CallGraph. 
      if (function == NULL)
        return false;

      return functions.count(function->getName().str());
    }

    static ExcludeFile& read() {
      ExcludeFile* req = new ExcludeFile();

      std::ifstream file("exclude.list", std::ios::in);
      if (!file.is_open())
        return *req;

      req->hasFile = true;

      std::string line;
      while (file.good()) {
        getline(file, line);
        req->functions.insert(line);
      }

      return *req;
    }

};

#endif // EXCLUDE_FILE_H
