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

  void addRelation(Value& from, Value& to) {
    if (_set.find(pair<Value*, Value*>(&from, &to)) != _set.end())
      return;

    _file << getNodeName(from) << " [label=\"" << getNodeCaption(from) << "\"" << getShape(from) << "];\n";
    _file << getNodeName(to) << " [label=\"" << getNodeCaption(to) << "\"" << getShape(from) << "];\n";
    _file << getNodeName(from) << " -> " << getNodeName(to) << ";\n";

    _set.insert(pair<Value*, Value*>(&from, &to));
  }

  string getShape(Value& v) {
    return (isa<Argument>(v) ? "shape=record, style=filled, color=lightblue" : "shape=record");
  }

  string _functionName;
  ofstream _file;
  set<pair<Value*, Value*> > _set;
};
