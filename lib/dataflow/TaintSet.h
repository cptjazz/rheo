#ifndef TAINTSET_H
#define TAINTSET_H

#include "RheoSmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Value.h"

using namespace llvm;


STATISTIC(NumTaintSetAdd, "calls to TaintSet.add()");
STATISTIC(NumTaintSetAddAll, "calls to TaintSet.addAll()");
STATISTIC(NumTaintSetRemove, "calls to TaintSet.remove()");
STATISTIC(NumTaintSetContains, "calls to TaintSet.contains()");
STATISTIC(NumTaintSetIntersect, "calls to TaintSet.intersect()");

class TaintSet {
  private:
    typedef RheoSmallPtrSet<const Value*, 32> InternalTaintSet;

    InternalTaintSet _taintSet;
    bool _taintSetChanged;

  public:
    typedef InternalTaintSet::const_iterator const_iterator;

    TaintSet() {
      resetChangedFlag();
    }

    inline void resetChangedFlag() {
      _taintSetChanged = false;
    }

    inline void clear() {
      _taintSet.clear();
    }

    /**
     * Add the provided Value to the provided TaintSet and
     * switch _taintSetChanged flag if the taint is new
     */
    inline bool add(const Value& v) {
      ++NumTaintSetAdd;
      bool isNew = _taintSet.insert(&v);
      _taintSetChanged |= isNew;
      return isNew;
    }

    inline bool contains(const Value& val) const {
      ++NumTaintSetContains;
      return _taintSet.count(&val);
    }

    /**
     * @param set Second set
     * @param intersect Result of set intersection
     */
    inline void intersect(const TaintSet& set, TaintSet& intersect) const {
      ++NumTaintSetIntersect;
      _taintSet.intersect(set._taintSet, intersect._taintSet);
    }

    inline void remove(const Value& val) {
      ++NumTaintSetRemove;
      _taintSet.erase(&val);
      _taintSetChanged = true;
    }

    inline void addAll(const TaintSet& set) {
      ++NumTaintSetAddAll;

      bool setChanged = _taintSet.insertAll(set._taintSet);
      _taintSetChanged |= setChanged;
    }
    
    inline int size() const {
      return _taintSet.size();
    }

    inline bool hasChanged() const {
      return _taintSetChanged;
    }

    inline void printTo(raw_ostream& stream) const {
      for (auto i : _taintSet)
        stream << *i << " | ";

      stream << "\n";
    } 

    inline InternalTaintSet::const_iterator begin() const {
      return _taintSet.begin();
    }

    inline InternalTaintSet::const_iterator end() const {
      return _taintSet.end();
    }
};

typedef TaintSet ReturnSet;

#endif // TAINTSET_H
