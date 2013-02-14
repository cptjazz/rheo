#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <llvm/Instruction.h>
#include <llvm/Value.h>
#include <llvm/ADT/StringMap.h>
#include <set>

using namespace std;
using namespace llvm;

class GraphExporter {
  public:

  GraphExporter(string functionName) {
    _functionName = functionName;
    _file.open((functionName + ".dot").c_str(), ios::out);
   
    _file << "digraph \"" << functionName << "\" { \n";
    _file << "  size =\"10,10\";\n";
    _file << "  label = \"" << functionName << "\";\n";
  }

  ~GraphExporter() {
    _file << "}\n";
    _file.close();
  } 

  string getNodeName(Value& i) {
    stringstream ss;
    ss << "_" << (long)(&i);
    return ss.str();
  }

  string getNodeCaption(Value& v) {
    stringstream ss;
    ss << v.getName().str() << "_" << (long)(&v);
    return ss.str();
  }

  void addInOutNode(Value& inout) {
    _nodes.erase(&inout);
    _file << getNodeName(inout) << " [label=\"" << getNodeCaption(inout) << "\"" << getInOutNodeShape(inout) << "];\n";
    _nodes.insert(&inout);
  }

  void addInNode(Value& in) {
    _nodes.erase(&in);
    _file << getNodeName(in) << " [label=\"" << getNodeCaption(in) << "\"" << getInNodeShape(in) << "];\n";
    _nodes.insert(&in);
  }

  void addOutNode(Value& out) {
    _nodes.erase(&out);
    _file << getNodeName(out) << " [label=\"" << getNodeCaption(out) << "\"" << getOutNodeShape(out) << "];\n";
    _nodes.insert(&out);
  }

  void addRelation(Value& from, Value& to) {
    if (_nodes.find(&from) == _nodes.end()) {
      _file << getNodeName(from) << " [label=\"" << getNodeCaption(from) << "\"" << getShape(from) << "];\n";
      _nodes.insert(&from);
    }

    if (_nodes.find(&to) == _nodes.end()) {
      _file << getNodeName(to) << " [label=\"" << getNodeCaption(to) << "\"" << getShape(from) << "];\n";
      _nodes.insert(&to);
    }

    if (_pairs.find(pair<Value*, Value*>(&from, &to)) == _pairs.end()) {
      _file << getNodeName(from) << " -> " << getNodeName(to) << ";\n";
      _pairs.insert(pair<Value*, Value*>(&from, &to));
    }
  }

  string getShape(Value& v) {
    return "shape=record";
  }

  string getInOutNodeShape(Value& v) {
    return "shape=record, style=filled, color=yellow";
  }

  string getInNodeShape(Value& v) {
    return "shape=record, style=filled, color=lightblue";
  }

  string getOutNodeShape(Value& v) {
    return "shape=record, style=filled, color=pink";
  }

  string _functionName;
  ofstream _file;
  set<pair<Value*, Value*> > _pairs;
  set<Value*> _nodes;
};
