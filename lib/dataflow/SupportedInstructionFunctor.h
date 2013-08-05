#pragma once
#include "Core.h"

struct SupportedInstructionFunctor {
  set<unsigned int> unsupportedInstructions;

  inline bool operator()(const Value& inst) {
    if (!isa<Instruction>(inst))
      return true;

    return !unsupportedInstructions.count(cast<Instruction>(inst).getOpcode());
  }

  void registerUnsupportedInstruction(const unsigned int opcode) {
    unsupportedInstructions.insert(opcode);
  }
};
