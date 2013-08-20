#ifndef HASHMAP_H
#define HASHMAP_H

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"

/*
STATISTIC(NumBucket0, "Bucket 0");
STATISTIC(NumBucket1, "Bucket 1");
STATISTIC(NumBucket2, "Bucket 2");
STATISTIC(NumBucket3, "Bucket 3");
STATISTIC(NumBucket4, "Bucket 4");
STATISTIC(NumBucket5, "Bucket 5");
STATISTIC(NumBucket6, "Bucket 6");
STATISTIC(NumBucket7, "Bucket 7");
STATISTIC(NumBucket8, "Bucket 8");
STATISTIC(NumBucket9, "Bucket 9");
STATISTIC(NumBucket10, "Bucket 10");
STATISTIC(NumBucket11, "Bucket 11");
STATISTIC(NumBucket12, "Bucket 12");
STATISTIC(NumBucket13, "Bucket 13");
STATISTIC(NumBucket14, "Bucket 14");
STATISTIC(NumBucket15, "Bucket 15");
*/
STATISTIC(NumPlusPlus, "HashMapIterator++");

template<class T>
class HashMap;

#define SIZE 4

template<class T>
class HashMapIterator {

  private:
    typedef typename llvm::SmallPtrSet<T*, 64> CollisionListType;
    const HashMap<T>& map;

    size_t currentPos;
    typename CollisionListType::const_iterator currentIter;

  public:

    HashMapIterator(const HashMap<T>& m, size_t cP, typename CollisionListType::const_iterator cI) 
      : map(m), currentPos(cP), currentIter(cI) { }

    HashMapIterator& operator++() {
      ++NumPlusPlus;

      const typename CollisionListType::const_iterator end = map.map[currentPos].end();

      if (currentIter != end) {
        ++currentIter;

        if (currentIter != end)
          return *this;
      }

      while (currentIter == map.map[currentPos].end()) {
        ++currentPos;

        if (currentPos >= SIZE) {
          --currentPos;
          return *this;
        }

        currentIter = map.map[currentPos].begin();
      }

      return *this;
    }

    bool operator!=(const HashMapIterator& other) {
      return !(operator==(other));
    }

    bool operator==(const HashMapIterator other) {
      return currentPos == other.currentPos && currentIter == other.currentIter;
    }

    T* operator*() {
      return *currentIter;
    }
};

template<class T>
class HashMap {
  private:
    typedef llvm::SmallPtrSet<T*, 64> CollisionListType;

    CollisionListType map[SIZE];
    size_t elemCount;

    inline unsigned int getHash(T* x) const {
      unsigned int hash = (SIZE - 1) & ((unsigned long)(x)>>8);
/*
      switch(hash) {
        case 0:
          ++NumBucket0;
          break;

        case 1:
          ++NumBucket2;
          break;

        case 2:
          ++NumBucket2;
          break;

        case 3:
          ++NumBucket3;
          break;

        case 4:
          ++NumBucket4;
          break;

        case 5:
          ++NumBucket5;
          break;

        case 6:
          ++NumBucket6;
          break;

        case 7:
          ++NumBucket7;
          break;

        case 8:
          ++NumBucket8;
          break;

        case 9:
          ++NumBucket9;
          break;

        case 10:
          ++NumBucket10;
          break;

        case 11:
          ++NumBucket11;
          break;

        case 12:
          ++NumBucket12;
          break;

        case 13:
          ++NumBucket13;
          break;

        case 14:
          ++NumBucket14;
          break;

        case 15:
          ++NumBucket15;
          break;
      }
      */

      return hash;
    }

  public:
    friend class HashMapIterator<T>;
    typedef HashMapIterator<T> const_iterator;

    HashMap() {
      elemCount = 0;
    }

    inline bool insert(T* x) {
      unsigned int hash = getHash(x);
      if (map[hash].insert(x)) {
        ++elemCount;
        return true;
      }

      return false;
    }

    inline void insertAll(const HashMap& other) {
      for (unsigned int i = 0; i < SIZE; ++i) {
        map[i].insert(other.map[i].begin(), other.map[i].end());
      }
    }

    inline bool count(T* x) const {
      unsigned int hash = getHash(x);
      return map[hash].count(x);
    }

    inline bool erase(T* x) {
      unsigned int hash = getHash(x);

      if (map[hash].erase(x)) {
        --elemCount;
        return true;
      }

      return false;
    }

    inline void clear() {
      elemCount = 0;

      for (unsigned int i = 0; i < SIZE; ++i)
        map[i].clear();
    }

    inline void intersect(const HashMap& other, HashMap& result) const {
      for (size_t j = 0; j < SIZE; ++j) {
        for (typename CollisionListType::const_iterator i = map[j].begin(), e = map[j].end(); i != e; ++i) {
          if (other.map[j].count(*i)) {
            result.map[j].insert(*i);
            ++result.elemCount;
          }
        }
      }
    }

    inline size_t size() const {
      return elemCount;
    }

    inline bool empty() const {
      return size() == 0;
    }

    inline const_iterator begin() const {
      for (size_t i = 0; i < SIZE; ++i) {
        if (map[i].begin() != map[i].end()) {
          return const_iterator(*this, i, map[i].begin());
        }
      }

      return end();
    }

    inline const_iterator end() const {
      return const_iterator(*this, SIZE - 1, map[SIZE - 1].end());
    }
};

#endif // HASHMAP_H
