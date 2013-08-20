#ifndef TAINTSET_H
#define TAINTSET_H

#include "HashMap.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Value.h"

using namespace llvm;

typedef HashMap<const Value> InternalTaintSet;

STATISTIC(NumTaintSetAdd, "calls to TaintSet.add()");
STATISTIC(NumTaintSetRemove, "calls to TaintSet.remove()");
STATISTIC(NumTaintSetContains, "calls to TaintSet.contains()");

class TaintSet {

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
    inline void add(const Value& v) {
      ++NumTaintSetAdd;
      _taintSetChanged |= _taintSet.insert(&v);
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
      _taintSet.intersect(set._taintSet, intersect._taintSet);
    }

    inline void remove(const Value& val) {
      ++NumTaintSetRemove;
      _taintSet.erase(&val);
    }

    inline void addAll(const TaintSet& set) {
      _taintSet.insertAll(set._taintSet);
      _taintSetChanged = true;
    }
    
    inline int size() const {
      return _taintSet.size();
    }

    inline bool hasChanged() const {
      return _taintSetChanged;
    }

    inline void printTo(raw_ostream& stream) const {
      for (InternalTaintSet::const_iterator i = _taintSet.begin(), e = _taintSet.end(); i != e; ++i) {
        stream << **i << " | ";
      }

      stream << "\n";
    } 

    inline InternalTaintSet::const_iterator begin() const {
      return _taintSet.begin();
    }

    inline InternalTaintSet::const_iterator end() const {
      return _taintSet.end();
    }

  private:
    InternalTaintSet _taintSet;
    bool _taintSetChanged;
};

typedef TaintSet ReturnSet;

#endif // TAINTSET_H
