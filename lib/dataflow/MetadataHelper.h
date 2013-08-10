#ifndef METADATAHELPER_H
#define METADATAHELPER_H

#include "Core.h"
#include "llvm/DebugInfo.h"
#include <sstream>


class MetadataHelper {
  public: 
    static bool hasMetadata(const Instruction& inst) {
      return inst.getMetadata("dbg") != NULL;
    }

    static string getFileAndLineNumber(const Instruction& inst) {
      stringstream stream;

      if (MDNode *n = inst.getMetadata("dbg")) {
        DILocation loc(n);
        unsigned int line = loc.getLineNumber();
        StringRef file = loc.getFilename();
        stream << file.str() << ":" << line;
      } 

      return stream.str();
    }
};

#endif // METADATAHELPER_H
