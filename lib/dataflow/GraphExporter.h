#ifndef GRAPHEXPORTER_H
#define GRAPHEXPORTER_H

#include <cstring>
#include <iostream>
#include <fstream>
#include <set>
#include <llvm/Value.h>

using namespace std;
using namespace llvm;

class GraphExporter {

public:
  GraphExporter(string functionName);
  ~GraphExporter();

  void addInOutNode(Value& inout);
  void addInNode(Value& in);
  void addOutNode(Value& out);
  void addRelation(Value& from, Value& to);

private:
  string _functionName;
  ofstream _file;
  set<pair<Value*, Value*> > _pairs;
  set<Value*> _nodes;

  string getShape(Value& v);
  string getInOutNodeShape(Value& v);
  string getInNodeShape(Value& v);
  string getOutNodeShape(Value& v);
  string getNodeName(Value& i);
  string getNodeCaption(Value& v);
};

#endif
