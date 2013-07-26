#ifndef TAINT_H
#define TAINT_H

#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"

using namespace llvm;

enum TaintType {
  infered,
  generated
};

struct Taint {
  private:
  Taint(const Value* val, TaintType t) : value(*val), type(t){
  }

  public:
  const Value& value;
  const TaintType type;

  inline static const Taint& make_generated(const Value* value) {
    return *(new Taint(value, generated));
  }

  inline static const Taint& make_generated(const Value& value) {
    return make_generated(&value);
  }

  inline static const Taint& make_infered(const Value* value) {
    return *(new Taint(value, infered));
  }

  inline static const Taint& make_infered(const Value& value) {
    return make_infered(&value);
  }

  template<class T>
  inline bool isa() const {
    return llvm::isa<T>(value);
  }

  inline bool operator<(const Taint& r) const {
    return &value < &(r.value);
  }

  inline bool operator==(const Taint& r) const {
    return &value == &(r.value);
  }
};

struct TaintPair {
  public:
  const Taint& source;
  const Taint& sink;

  static const TaintPair& make(const Taint& src, const Taint& snk) {
    return *(new TaintPair(src, snk));
  }

  static const TaintPair& make(const Taint* src, const Taint* snk) {
    return make(*src, *snk);
  }

  private:
  TaintPair(const Taint& src, const Taint& snk) : source(src), sink(snk) { }
};

#endif // TAINT_H
