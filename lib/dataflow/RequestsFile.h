#ifndef REQUESTS_FILE_H
#define REQUESTS_FILE_H

#include "Core.h"
#include <string>
#include <fstream>
#include "llvm/Function.h"

class RequestsFile {
  private:
    set<string> functions;
    bool hasFile;

    RequestsFile() { 
      hasFile = false;
    }

  public:
    inline bool includeFunction(const Function* function) {
      // If we do not have any special requests
      // we analyse everything.
      if (!hasFile)
        return true;

      // This handles the virtual root node
      // of the CallGraph. Since we have a file
      // we do not want this to be analysed 
      // (would cause cascading everything)
      if (function == NULL)
        return false;

      return functions.count(function->getName().str());
    }

    static RequestsFile& read() {
      RequestsFile* req = new RequestsFile();

      ifstream file("requests.list", ios::in);
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

#endif // REQUESTS_FILE_H
