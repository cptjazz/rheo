#include "GraphExporter.h"
#include <sstream>
#include <llvm/Instruction.h>
#include <llvm/Support/raw_ostream.h>


void GraphExporter::initialiseFile() {
  _file.open((_functionName + ".dot").c_str(), ios::out);

  _file << "digraph \"" << _functionName << "\" { \n";
  _file << "  graph [";
  _file << "    splines=\"true\",\n";
  _file << "    label=\"" << _functionName << "\"\n";
  _file << "  ];\n";

  _file << "  edge [ fontsize=8, arrowsize=0.7 ];\n";
}

GraphExporter::~GraphExporter() {
  _file << "}\n";
  _file.close();
} 

void GraphExporter::addInOutNode(Value& inout) {
  _nodes.erase(&inout);
  _file << "  { rank=source;\n";
  _file << "    " << getNodeName(inout) << " [label=\"" << getNodeCaption(inout) 
        << "\"" << getInOutNodeShape(inout) << "];\n";
  _file << "  }\n";
  _nodes.insert(&inout);
}

void GraphExporter::addInNode(Value& in) {
  _nodes.erase(&in);
  _file << "  { rank=source;\n";
  _file << "    " << getNodeName(in) << " [label=\"" << getNodeCaption(in) << "\"" 
        << getInNodeShape(in) << "];\n";
  _file << "  }\n";
  _nodes.insert(&in);
}

void GraphExporter::addOutNode(Value& out) {
  _nodes.erase(&out);
  _file << "  { rank=sink;\n";
  _file << "    " << getNodeName(out) << " [label=\"" << getNodeCaption(out) << "\"" 
        << getOutNodeShape(out) << "];\n";
  _file << "  }\n";
  _nodes.insert(&out);
}

void GraphExporter::addBlockNode(Value& b) {
  _nodes.erase(&b);
  _file << "    " << getNodeName(b) << " [label=\"" << getNodeCaption(b) << "\"" 
        << ", weight=3];\n";
  _nodes.insert(&b);
}

void GraphExporter::addRelation(Value& from, Value& to, string reason) {
  if (_nodes.find(&from) == _nodes.end()) {
    _file << getNodeName(from) << " [label=\"" << getNodeCaption(from) << "\"" << getShape(from) << "];\n";
    _nodes.insert(&from);
  }

  if (_nodes.find(&to) == _nodes.end()) {
    _file << getNodeName(to) << " [label=\"" << getNodeCaption(to) << "\"" << getShape(from) << "];\n";
    _nodes.insert(&to);
  }

  if (_pairs.find(pair<Value*, Value*>(&from, &to)) == _pairs.end()) {
    _file << getNodeName(from) << " -> " << getNodeName(to) 
          << " [label=\"" << getLabel(reason) << "\", "
          << "style=\"" << getLineStyle(reason) << "\""
          <<  "];\n";

    _pairs.insert(pair<Value*, Value*>(&from, &to));
  }
}

string GraphExporter::getLabel(string reason) const {
  return reason == "block-taint" ? "" : reason;
}

string GraphExporter::getLineStyle(string reason) const {
  return reason == "block-taint" ? "dotted" :
         reason == "out-init" ? "dashed" : "solid";
}

string GraphExporter::getShape(Value& v) const {
  return "shape=record";
}

string GraphExporter::getInOutNodeShape(Value& v) const {
  return "shape=record, style=filled, color=yellow";
}

string GraphExporter::getInNodeShape(Value& v) const {
  return "shape=record, style=filled, color=lightblue";
}

string GraphExporter::getOutNodeShape(Value& v) const {
  return "shape=record, style=filled, color=pink";
}

string GraphExporter::getNodeName(Value& i) const {
  stringstream ss;
  ss << "_" << (long)(&i);
  return ss.str();
}

string GraphExporter::getNodeCaption(Value& v) const {
   if (!v.hasName()) {
    string s;
    raw_string_ostream rstr(s);
    rstr << v;
    return rstr.str();
  }

  stringstream ss;
  ss << v.getName().str();
  return ss.str();
}
