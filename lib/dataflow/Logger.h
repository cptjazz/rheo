#ifndef LOGGER_H
#define LOGGER_H

#include "llvm/Support/raw_ostream.h"
#include "Core.h"


class Logger {
  private:
    raw_ostream& errStream;
    raw_ostream& nullStream;

  public:
    Logger(raw_ostream& errStream, raw_ostream& nullStream) : errStream(errStream), nullStream(nullStream)  { }

    inline raw_ostream& debug() const {
      return errStream;
    }

    inline raw_ostream& profile() const {
      errStream << "~~~PROFILING: ";
      return errStream;
    }

    inline raw_ostream& error() const {
      errStream << "__error:";
      return errStream;
    }

    inline raw_ostream& info() const {
      errStream << "__info:";
      return errStream;
    }

    inline raw_ostream& output() const {
      return errStream;
    }

    inline raw_ostream& null() const {
      return nullStream;
    }
};

#endif // LOGGER_H
