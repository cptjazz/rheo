#ifndef RHEOSMALLPTRSET_H
#define RHEOSMALLPTRSET_H

#include "llvm/ADT/SmallPtrSet.h"

using namespace llvm;

template<typename PtrType, unsigned SmallSize>
class RheoSmallPtrSet : public SmallPtrSet<PtrType, SmallSize> {
  public:
    void intersect(const SmallPtrSet<PtrType, SmallSize>& other, SmallPtrSet<PtrType, SmallSize>& result) const {
      for (SmallPtrSetIterator<PtrType> i = this->begin(), e = this->end(); i != e; ++i) {
        if (other.count(*i))
          result.insert(*i);
      }
    }

    bool insertAll(const SmallPtrSet<PtrType, SmallSize>& other) {
      size_t s = this->size();
      this->insert(other.begin(), other.end());
      return s != this->size();
    }

};

#endif // RHEOSMALLPTRSET_H
