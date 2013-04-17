#ifndef NULLGRAPHEXPORTER_H
#define NULLGRAPHEXPORTER_H

#include <cstring>
#include <iostream>
#include <fstream>
#include <set>
#include <llvm/Value.h>
#include <llvm/Function.h>
#include "GraphExporter.h"

using namespace std;
using namespace llvm;

class NullGraphExporter : public GraphExporter {

public:
  inline virtual void addInOutNode(const Value& inout) { }
  inline virtual void addInNode(const Value& in) { }
  inline virtual void addOutNode(const Value& out) { }
  inline virtual void addBlockNode(const Value& b) { }
  inline virtual void addCallNode(const Function& f) { }
  inline virtual void addRelation(const Value& from, const Value& to, string reason = "") { }
};

#endif
