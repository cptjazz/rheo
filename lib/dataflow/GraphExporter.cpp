#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <llvm/Instruction.h>

using namespace std;
using namespace llvm;

class GraphExporter {
  public:

  GraphExporter(string functionName) {
    _functionName = functionName;
    _file.open((functionName + ".dot").c_str(), ios::out);
   
    _file << "digraph \"" << functionName << "\" { \n";
    _file << "  label = \"" << functionName << "\";\n";
  }

  ~GraphExporter() {
    _file << "}\n";
    _file.close();
  } 

  string getNodeName(Value& i) {
    stringstream ss;
    ss << "_" << i.getName().str() << "_" << (long)(&i);
    return ss.str();
  }

  void addRelation(Value& from, Value& to) {
    _file << getNodeName(from) << " [label=\"" << from.getName().str() << "\"];\n";
    _file << getNodeName(to) << " [label=\"" << to.getName().str() << "\"];\n";
    _file << getNodeName(from) << " -> " << getNodeName(to) << ";\n";
  }

  string _functionName;
  ofstream _file;
};
