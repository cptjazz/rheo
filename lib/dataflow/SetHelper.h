#ifndef SET_HELPER_H
#define SET_HELPER_H

#include "Core.h"
#include "Logger.h"


class SetHelper {
  private:
    const Logger& logger;
    bool resultSetChanged;

    void intersectSets(const Value& arg, const TaintSet argTaintSet);

  public:
    TaintMap returnStatements;
    TaintMap arguments;
    ResultSet resultSet;

    SetHelper(const Logger& logger) : logger(logger) { }

    bool hasResultSetChanged() const { return resultSetChanged; }

    void resetResultSetChanged() { resultSetChanged = 0; }

    void buildResultSet();

    void addTaint(const Value &tainter, const Value &taintee);
};

#endif // SET_HELPER_H
