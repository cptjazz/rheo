#ifndef TAINTSET_H
#define TAINTSET_H

#include "llvm/Support/raw_ostream.h"
//#include "llvm/ADT/SmallPtrSet.h"
#include <map>
#include "Taint.h"

using namespace llvm;
using namespace std;


typedef map<const Value*, const Taint*> InternalTaintSet;

class TaintSet {

  public:
    typedef InternalTaintSet::const_iterator const_iterator;

    TaintSet() {
      resetChangedFlag();
    }

    ~TaintSet() {

      // TODO: free memory for elements?
    }

    inline void resetChangedFlag() {
      _taintSetChanged = false;
    }

    /**
     * Add the provided Value to the provided TaintSet and
     * switch _taintSetChanged flag if the taint is new
     */
    inline void add(const Taint& v) {
      _taintSetChanged |= _taintSet.insert(make_pair(&(v.value), &v)).second;
    }

    inline bool contains(const Value& val) const {
      return _taintSet.count(&val);
    }

    inline bool contains(const Taint& t) const {
      return contains(t.value);
    }

    /**
     * @param set Second set
     * @param intersect Result of set intersection
     */
    inline void intersect(const TaintSet& set, TaintSet& intersect) const {
      for (InternalTaintSet::const_iterator i = _taintSet.begin(), e = _taintSet.end(); i != e; ++i) {
        if (set._taintSet.count(i->first))
          intersect._taintSet.insert(*i);
      }
    }

    inline void remove(const Value& val) {
      _taintSet.erase(&val);
    }

    inline void remove(const Taint& t) {
      remove(t.value);
    }

    inline void addAll(TaintSet& set) {
      for (InternalTaintSet::const_iterator t_i = set._taintSet.begin(), t_e = set._taintSet.end(); t_i != t_e; ++t_i) {
        add(*t_i->second);
      }
    }
    
    inline int size() const {
      return _taintSet.size();
    }

    inline bool hasChanged() const {
      return _taintSetChanged;
    }

    inline void printTo(raw_ostream& stream) const {
      for (InternalTaintSet::const_iterator i = _taintSet.begin(), e = _taintSet.end(); i != e; ++i) {
        const Taint& t = *i->second;
        stream << "{" << t.value << ", val_addr: " << &(t.value) << ", t_addr:" << &t << "} | ";
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
