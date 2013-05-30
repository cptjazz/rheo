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
  inline void addInOutNode(const Value& inout) { }
  inline void addInNode(const Value& in) { }
  inline void addOutNode(const Value& out) { }
  inline void addBlockNode(const Value& b) { }
  inline void addCallNode(const Value& f) { }
  inline void addRelation(const Value& from, const Value& to, string reason = "") { }
};

#endif
