#ifndef SPECIALTAINTINSTRUCTION_H
#define SPECIALTAINTINSTRUCTION_H

#include "Core.h"
#include "MetadataHelper.h"
#include <string>
#include <sstream>

struct SpecialTaint {
  public:
    const Value* value;
    TaintType type;
    ValueSet affectedValues;

    SpecialTaint(const Value* v, TaintType t) : value(v), type(t) { }

    void registerAffectedValue(const Value& v) {
      affectedValues.insert(&v);
    }

    static SpecialTaint createNullTaint() {
      return SpecialTaint(NULL, NoTaint);
    }
};

class SpecialTaintInstruction {
  private:
    LLVMContext& _llvmContext;
    const string _functionName;
    TaintType _taintType;

    string createUniqueNameForInstruction(const CallInst& call) {
      stringstream stream;

      if (MetadataHelper::hasMetadata(call)) {
        stream << "@" << MetadataHelper::getFileAndLineNumber(call);
      } else {
        if (call.hasName())
          stream << "@" << call.getName().str() << ":";
        
        stream << (long)(&call);
      }

      return stream.str();
    }

    const Value* createValueWithName(string name, CallInst& call) {
        return BasicBlock::Create(_llvmContext, "+" + name + createUniqueNameForInstruction(call));
    }

  protected:
    SpecialTaint createSpecialTaint(string name, CallInst& call) {
      return SpecialTaint(createValueWithName(name, call), getTaintType());
    }

    void addAffectedValue(TaintSet& taints, SpecialTaint st, const Value& val) {
      taints.add(val);
      st.registerAffectedValue(val);
    }

  public:
    SpecialTaintInstruction(LLVMContext& context, const string fname, TaintType type) 
      : _llvmContext(context), _functionName(fname), _taintType(type) { }

    bool canHandle(const Function& func) const {
      return (func.getName().equals(_functionName));
    }

    string getFunctionName() const {
      return _functionName;
    }

    TaintType getTaintType() const {
      return _taintType;
    }

    virtual const SpecialTaint handleInstruction(CallInst& call, TaintSet& taints) = 0;
};

#endif // SPECIALTAINTINSTRUCTION_H
