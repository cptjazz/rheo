#ifndef METADATAHELPER_H
#define METADATAHELPER_H

#include "Core.h"
#include "llvm/DebugInfo.h"


class MetadataHelper {
  public: 
    static bool hasMetadata(const Instruction& inst) {
      return inst.getMetadata("dbg") != NULL;
    }

    static StringRef getFileAndLineNumber(const Instruction& inst) {
      std::string s;
      raw_string_ostream stream(s);

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
