#include "GraphExporter.h"
#include <sstream>
#include <algorithm>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Support/raw_ostream.h>


void GraphExporter::initialiseFile() {
  _file.open((_functionName + ".dot").c_str(), ios::out);

  _file << "digraph \"" << _functionName << "\" { \n";
  _file << "  graph [\n";
  _file << "    splines=\"true\",\n";
  _file << "    label=\"" << _functionName << "\"\n";
  _file << "  ];\n";

  _file << "  edge [ fontsize=8, arrowsize=0.7 ];\n";
}

GraphExporter::~GraphExporter() {
  _file << "}\n";
  _file.close();
} 

void GraphExporter::addInOutNode(const Value& inout) {
  if (_nodes.count(&inout))
    return;

  _file << "  { rank=source;\n";
  _file << "    " << getNodeName(inout) << " [label=\"" << getNodeCaption(inout) 
        << "\"" << getInOutNodeShape(inout) << "];\n";
  _file << "  }\n";
  _nodes.insert(&inout);
}

void GraphExporter::addInNode(const Value& in) {
  if (_nodes.count(&in))
    return;

  _file << "  { rank=source;\n";
  _file << "    " << getNodeName(in) << " [label=\"" << getNodeCaption(in) << "\"" 
        << getInNodeShape(in) << "];\n";
  _file << "  }\n";
  _nodes.insert(&in);
}

void GraphExporter::addOutNode(const Value& out) {
  if (_nodes.count(&out))
    return;

  _file << "  { rank=sink;\n";
  _file << "    " << getNodeName(out) << " [label=\"" << getNodeCaption(out) << "\"" 
        << getOutNodeShape(out) << "];\n";
  _file << "  }\n";
  _nodes.insert(&out);
}

void GraphExporter::addBlockNode(const Value& b) {
  if (_nodes.count(&b))
    return;

  _file << "    " << getNodeName(b) << " [label=\"" << getNodeCaption(b) << "\"" 
        << ", weight=3];\n";
  _nodes.insert(&b);
}

void GraphExporter::addCallNode(const Value& f) {
  if (_nodes.count(&f))
    return;

  _file << "    " << getNodeName(f) << " [label=\"" << getNodeCaption(f) << "\"" 
        << ", weight=3, shape=polygon, skew=0.5];\n";
  _nodes.insert(&f);
}

void GraphExporter::addRelation(const Value& from, const Value& to, string reason) {
  if (_nodes.find(&from) == _nodes.end()) {
    _file << getNodeName(from) << " [label=\"" << getNodeCaption(from) << "\", " << getShape(from) << "];\n";
    _nodes.insert(&from);
  }

  if (_nodes.find(&to) == _nodes.end()) {
    _file << getNodeName(to) << " [label=\"" << getNodeCaption(to) << "\", " << getShape(from) << "];\n";
    _nodes.insert(&to);
  }

  if (_pairs.find(make_pair(&from, &to)) == _pairs.end()) {
    _file << getNodeName(from) << " -> " << getNodeName(to) 
          << " [label=\"" << getLabel(reason) << "\", "
          << "style=\"" << getLineStyle(reason) << "\""
          <<  "];\n";

    _pairs.insert(make_pair(&from, &to));
  }
}

string GraphExporter::getLabel(string reason) const {
  return reason == "block-taint" ? "" : reason;
}

string GraphExporter::getLineStyle(string reason) const {
  return reason == "block-taint" ? "dotted" :
         reason == "function indirection" ? "dashed" : "solid";
}

string GraphExporter::getShape(const Value& v) const {
  return "shape=record";
}

string GraphExporter::getInOutNodeShape(const Value& v) const {
  return "shape=record, style=filled, color=yellow";
}

string GraphExporter::getInNodeShape(const Value& v) const {
  return "shape=record, style=filled, color=lightblue";
}

string GraphExporter::getOutNodeShape(const Value& v) const {
  return "shape=record, style=filled, color=pink";
}

string GraphExporter::getNodeName(const Value& i) const {
  stringstream ss;
  ss << "_" << (long)(&i);
  return ss.str();
}

string GraphExporter::getNodeCaption(const Value& v) const {
   if (!v.hasName()) {
    if (isa<SwitchInst>(v))
      return "switch";
    else if (isa<BranchInst>(v))
      return "br";
    else {
      string s;
      raw_string_ostream rstr(s);
      rstr << v;
      s = rstr.str();
      replace( s.begin(), s.end(), '[', '(');
      replace( s.begin(), s.end(), ']', ')');
      replace( s.begin(), s.end(), '\n', ' ');
      return s;
    }
  }

  stringstream ss;
  ss << v.getName().str();
  return ss.str();
}
