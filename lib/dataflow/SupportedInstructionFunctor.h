#ifndef SUPPORTEDINSTRUCTIONFUNCTOR_H
#define SUPPORTEDINSTRUCTIONFUNCTOR_H

#include "Core.h"

struct SupportedInstructionFunctor {
  set<unsigned int> unsupportedInstructions;

  inline bool operator()(const Value& v) {
    if (const Instruction* inst = dyn_cast<Instruction>(&v))
      return !unsupportedInstructions.count(inst->getOpcode());

    return true;
  }

  void registerUnsupportedInstruction(const unsigned int opcode) {
    unsupportedInstructions.insert(opcode);
  }
};

#endif // SUPPORTEDINSTRUCTIONFUNCTOR_H
