#ifndef SPECIALTAINTINSTRUCTION_H
#define SPECIALTAINTINSTRUCTION_H

#include "Core.h"
#include "MetadataHelper.h"

struct SpecialTaint {
  const Value* specialTaintValue;
  TaintSet sources;
  TaintSet sinks;
  TaintSet aliases;
  TaintType type;

  static const SpecialTaint& Null;

  SpecialTaint(const Value* v, TaintType t) : specialTaintValue(v), type(t) { }

  void registerAlias(const Value& v) {
    aliases.add(v);
  }

  void registerSource(const Value& v) {
    sources.add(v);
  }

  void registerSelfAsSource() {
    registerSource(*specialTaintValue);
  }

  void registerSink(const Value& v) {
    sinks.add(v);
  }

  void registerSelfAsSink() {
    registerSink(*specialTaintValue);
  }

  static const SpecialTaint& createNullTaint() {
    return Null;
  }

  bool operator<(const SpecialTaint& r) const {
    return this < &r;
  }
};


class SpecialTaintInstruction {
  private:
    LLVMContext& _llvmContext;
    const StringRef _functionName;
    TaintType _taintType;

    std::string createUniqueNameForInstruction(const CallInst& call) {
      std::string s;
      raw_string_ostream stream(s);

      if (MetadataHelper::hasMetadata(call)) {
        stream << "@" << MetadataHelper::getFileAndLineNumber(call);
      } else {
        if (call.hasName())
          stream << "@" << call.getName().str() << ":";
        
        stream << (long)(&call);
      }

      return stream.str();
    }

    const Value* createValueWithName(std::string name, const CallInst& call) {
        return BasicBlock::Create(_llvmContext, "+" + name + createUniqueNameForInstruction(call));
    }

  protected:
    SpecialTaint& createSpecialTaint(std::string name, const CallInst& call) {
      return *(new SpecialTaint(createValueWithName(name, call), getTaintType()));
    }

  public:
    SpecialTaintInstruction(LLVMContext& context, const StringRef fname, TaintType type) 
      : _llvmContext(context), _functionName(fname), _taintType(type) { }

    bool canHandle(const Function& func) const {
      return (func.getName().equals(_functionName));
    }

    StringRef getFunctionName() const {
      return _functionName;
    }

    TaintType getTaintType() const {
      return _taintType;
    }

    virtual const SpecialTaint handleInstruction(const CallInst& call) = 0;
};

#endif // SPECIALTAINTINSTRUCTION_H
