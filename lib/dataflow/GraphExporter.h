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
  GraphExporter(string functionName) : _functionName(functionName) {
    initialiseFile();
  }

  ~GraphExporter();

  void addInOutNode(Value& inout);
  void addInNode(Value& in);
  void addOutNode(Value& out);
  void addRelation(Value& from, Value& to, string reason = "");

private:
  const string _functionName;
  ofstream _file;
  set<pair<Value*, Value*> > _pairs;
  set<Value*> _nodes;

  void initialiseFile();
  string getShape(Value& v) const;
  string getInOutNodeShape(Value& v) const;
  string getInNodeShape(Value& v) const;
  string getOutNodeShape(Value& v) const;
  string getNodeName(Value& i) const;
  string getNodeCaption(Value& v) const;
  string getLineStyle(string reason) const;
};

#endif
