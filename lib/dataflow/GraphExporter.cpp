#include "GraphExporter.h"
#include <sstream>
#include <llvm/Instruction.h>
#include <llvm/ADT/StringMap.h>


void GraphExporter::initialiseFile() {
  _file.open((_functionName + ".dot").c_str(), ios::out);

  _file << "digraph \"" << _functionName << "\" { \n";
  _file << "  size =\"10,10\";\n";
  _file << "  label = \"" << _functionName << "\";\n";
}

GraphExporter::~GraphExporter() {
  _file << "}\n";
  _file.close();
} 

void GraphExporter::addInOutNode(Value& inout) {
  _nodes.erase(&inout);
  _file << getNodeName(inout) << " [label=\"" << getNodeCaption(inout) << "\"" << getInOutNodeShape(inout) << "];\n";
  _nodes.insert(&inout);
}

void GraphExporter::addInNode(Value& in) {
  _nodes.erase(&in);
  _file << getNodeName(in) << " [label=\"" << getNodeCaption(in) << "\"" << getInNodeShape(in) << "];\n";
  _nodes.insert(&in);
}

void GraphExporter::addOutNode(Value& out) {
  _nodes.erase(&out);
  _file << getNodeName(out) << " [label=\"" << getNodeCaption(out) << "\"" << getOutNodeShape(out) << "];\n";
  _nodes.insert(&out);
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
          << " [label=\"" << reason << "\", style=\"" 
          << getLineStyle(reason) <<  "\"];\n";

    _pairs.insert(pair<Value*, Value*>(&from, &to));
  }
}

string GraphExporter::getLineStyle(string reason) const {
  return reason == "block-taint" ? "dotted" : "solid";
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
  stringstream ss;
  ss << v.getName().str() << "_" << (long)(&v);
  return ss.str();
}
