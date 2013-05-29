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

    raw_ostream& debug() const {
      DEBUG(return errStream);
      return nullStream;
    }

    raw_ostream& profile() const {
      IF_PROFILING(return debug());
      return nullStream;
    }

    raw_ostream& error() const {
      errStream << "__error:";
      return errStream;
    }

    raw_ostream& output() const {
      return errStream;
    }
};

#endif // LOGGER_H
