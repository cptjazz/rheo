#ifndef TAINTSET_H
#define TAINTSET_H

#include "llvm/Support/raw_ostream.h"

using namespace llvm;

typedef SmallPtrSet<const Value*, 32> InternalTaintSet;

class TaintSet {

  public:
    typedef InternalTaintSet::const_iterator const_iterator;

    TaintSet() {
      resetChangedFlag();
    }

    inline void resetChangedFlag() {
      _taintSetChanged = false;
    }

    /**
     * Add the provided Value to the provided TaintSet and
     * switch _taintSetChanged flag if the taint is new
     */
    inline void add(const Value& v) {
      if (_taintSet.insert(&v))
        _taintSetChanged = true;
    }

    inline bool contains(const Value& val) const {
      return _taintSet.count(&val);
    }

    /**
     * @param set Second set
     * @param intersect Result of set intersection
     */
    inline void intersect(const TaintSet& set, TaintSet& intersect) const {
      for (InternalTaintSet::const_iterator i = _taintSet.begin(), e = _taintSet.end(); i != e; ++i) {
        if (set.contains(**i))
          intersect.add(**i);
      }
    }

    inline void remove(const Value& val) {
      _taintSet.erase(&val);
    }

    inline void addAll(TaintSet& set) {
      for (InternalTaintSet::const_iterator t_i = set._taintSet.begin(), t_e = set._taintSet.end(); t_i != t_e; ++t_i) {
        add(**t_i);
      }
    }
    
    inline int size() const {
      return _taintSet.size();
    }

    inline bool hasChanged() const {
      return _taintSetChanged;
    }

    void printTo(raw_ostream& stream) const {
      /*for (InternalTaintSet::const_iterator i = _taintSet.begin(), e = _taintSet.end(); i != e; ++i) {
        DEBUG(stream << **i << " | ");
      }

      DEBUG(stream << "\n");*/
    } 

    InternalTaintSet::const_iterator begin() const {
      return _taintSet.begin();
    }

    InternalTaintSet::const_iterator end() const {
      return _taintSet.end();
    }

  private:
    InternalTaintSet _taintSet;
    bool _taintSetChanged;
};

typedef TaintSet ReturnSet;

#endif // TAINTSET_H
