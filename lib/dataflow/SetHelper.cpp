#include "SetHelper.h"
#include "Core.h"
#include "Helper.h"

void SetHelper::buildResultSet() {

  TaintMap::const_iterator arg_i = arguments.begin();
  TaintMap::const_iterator arg_e = arguments.end();

  for(; arg_i != arg_e; ++arg_i) {
    const Value& arg = *arg_i->first;
    const TaintSet& taintSet = arg_i->second;

    intersectSets(arg, taintSet);
  }
}


void SetHelper::intersectSets(const Value& arg, const TaintSet argTaintSet) {
  TaintMap::const_iterator ret_i = returnStatements.begin();
  TaintMap::const_iterator ret_e = returnStatements.end();

  for (; ret_i != ret_e; ++ret_i) {
    const Value& retval = *ret_i->first;
    const TaintSet retTaintSet = ret_i->second;

    if (&retval == &arg) {
      DEBUG(logger.debug() << "Skipping detected self-taint\n");
      continue;
    }

    DEBUG(logger.debug() << "Ret-set for `" << retval << "`:\n");
    DEBUG(retTaintSet.printTo(logger.debug()));

    IF_PROFILING(long t = Helper::getTimestamp());
    TaintSet intersect;
    argTaintSet.intersect(retTaintSet, intersect);
    IF_PROFILING(logger.profile() << "intersect() took " << Helper::getTimestampDelta(t) << " µs\n");

    if (intersect.size()) {
      addTaint(arg, retval);

      DEBUG(logger.debug() << "Values that lead to taint " << Helper::getValueNameOrDefault(arg) << " -> "
              << Helper::getValueNameOrDefault(retval) << ":\n");
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
  if (resultSet.insert(make_pair(&tainter, &taintee)).second) {
    resultSetChanged = true;
    DEBUG(logger.debug() << "Added taint. Result set changed.\n");
  }
}
