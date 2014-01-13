#include "SetHelper.h"
#include "Core.h"
#include "Helper.h"

void SetHelper::buildResultSet() {

  for (auto arg_i : arguments) {
    const Value& arg = *arg_i.first;
    const TaintSet& taintSet = arg_i.second;

    intersectSets(arg, taintSet);
  }
}


void SetHelper::intersectSets(const Value& arg, const TaintSet argTaintSet) {

  for (auto ret_i : returnStatements) {
    const Value& retval = *ret_i.first;
    const TaintSet retTaintSet = ret_i.second;

    DEBUG(logger.debug() << "Ret-set for `" << retval << "`:\n");
    DEBUG(retTaintSet.printTo(logger.debug()));

    IF_PROFILING(long t = Helper::getTimestamp());
    TaintSet intersect;
    retTaintSet.intersect(argTaintSet, intersect);
    IF_PROFILING(logger.profile() << "intersect() took " << Helper::getTimestampDelta(t) << " µs\n");

    if (intersect.size()) {
      addTaint(arg, retval);

      DEBUG(logger.debug() << "Values that lead to taint " << Helper::getValueName(arg) << " -> "
              << Helper::getValueName(retval) << ":\n");
      DEBUG(intersect.printTo(logger.debug()));
    }
  }
}


/**
 * Adds a taint to the final result set.
 * Also switches the _resultSetChanged flag if the taint is new.
 *
 * @param tainter The source of the taint (usually an argument)
 * @param taintee The sink of the taint (usually a return or out-pointer)
 */
void SetHelper::addTaint(const Value& tainter, const Value& taintee) {
  if (resultSet.insert(std::make_pair(&tainter, &taintee)).second) {
    resultSetChanged = true;
    DEBUG(logger.debug() << "Added taint. Result set changed.\n");
  }
}
