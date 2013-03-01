#include "Helper.h"
#include <llvm/Argument.h>
#include <llvm/GlobalVariable.h>

string Helper::getValueNameOrDefault(Value& v) {
  if (isa<Argument>(v) || isa<GlobalVariable>(v))
    return v.getName().str();
  else
    return "$_retval";
}

