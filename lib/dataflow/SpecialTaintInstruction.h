#ifndef SPECIALTAINTINSTRUCTION_H
#define SPECIALTAINTINSTRUCTION_H

#include "Core.h"
#include "MetadataHelper.h"

struct SpecialTaint {
  public:
    const Value* value;
    TaintType type;
    TaintSet affectedValues;

    static const SpecialTaint& Null;

    SpecialTaint(const Value* v, TaintType t) : value(v), type(t) { }

    void registerAffectedValue(const Value& v) {
      affectedValues.add(v);
    }

    static const SpecialTaint& createNullTaint() {
      return Null;
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

    void addAffectedValue(SpecialTaint& st, const Value& val) {
      st.registerAffectedValue(val);
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
