#ifndef GRAPHEXPORTER_H
#define GRAPHEXPORTER_H

#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <llvm/Value.h>
#include <llvm/Function.h>

using namespace std;
using namespace llvm;

class GraphExporter {

public:
  GraphExporter(string functionName) : _functionName(functionName) { }

  ~GraphExporter();

  void addInOutNode(const Value& inout);
  void addInNode(const Value& in);
  void addOutNode(const Value& out);
  void addBlockNode(const Value& b);
  void addCallNode(const Value& f);
  void addRelation(const Value& from, const Value& to, string reason = "");

  void addCGFunction(const Function& f);
  void addCGCall(const Function& from, const Function& to);
    
  void init();

protected:
  GraphExporter() { }

private:
  const string _functionName;
  ofstream _file;
  set<pair<const Value*, const Value*> > _pairs;
  set<const Value*> _nodes;

  string getShape(const Value& v) const;
  string getInOutNodeShape(const Value& v) const;
  string getInNodeShape(const Value& v) const;
  string getOutNodeShape(const Value& v) const;
  string getNodeName(const Value& i) const;
  string getNodeCaption(const Value& v) const;
  string getLineStyle(string reason) const;
  string getLabel(string reason) const;
};

#endif
