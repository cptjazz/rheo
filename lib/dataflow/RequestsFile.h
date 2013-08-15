#ifndef REQUESTS_FILE_H
#define REQUESTS_FILE_H

#include "Core.h"
#include <string>
#include <fstream>

class RequestsFile {
  private:
    std::set<std::string> functions;
    bool hasFile;

    RequestsFile() { 
      hasFile = false;
    }

  public:
    inline bool includesFunction(const Function* function) {
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

      std::ifstream file("requests.list", std::ios::in);
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

#endif // REQUESTS_FILE_H
