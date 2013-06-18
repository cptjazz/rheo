#ifndef EXCLUDE_FILE_H
#define EXCLUDE_FILE_H

#include "Core.h"
#include <string>
#include <fstream>

class ExcludeFile {
  private:
    set<string> functions;
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

      ifstream file("exclude.list", ios::in);
      if (!file.is_open())
        return *req;

      req->hasFile = true;

      string line;
      while (file.good()) {
        getline(file, line);
        req->functions.insert(line);
      }

      return *req;
    }

};

#endif // EXCLUDE_FILE_H
