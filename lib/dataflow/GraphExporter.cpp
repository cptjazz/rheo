#include "GraphExporter.h"
#include "MetadataHelper.h"
#include <sstream>
#include "llvm/IR/Instructions.h"


void GraphExporter::init() {
  _file.open((_functionName + ".dot").c_str(), std::ios::out);

  _file << "digraph \"" << _functionName << "\" { \n";
  _file << "  graph [\n";
  _file << "    splines=\"true\",\n";
  _file << "    label=\"" << _functionName << "\"\n";
  _file << "    bgcolor=\"white\"\n";
  _file << "  ];\n";

  _file << "  edge [ fontsize=8, arrowsize=0.7 ];\n";
}

GraphExporter::~GraphExporter() {
  _file << "}\n";
  _file.close();
} 

void GraphExporter::addInOutNode(const Value& inout) {
  if (_nodes.count(getNodeName(inout)))
    return;

  _file << "  { rank=source;\n";
  _file << "    " << getNodeName(inout) << " [label=\"" << getNodeCaption(inout) 
        << "\"" << getInOutNodeShape(inout) << "];\n";
  _file << "  }\n";

  _nodes.insert(getNodeName(inout));
}

void GraphExporter::addInNode(const Value& in) {
  if (_nodes.count(getNodeName(in)))
    return;

  _file << "  { rank=source;\n";
  _file << "    " << getNodeName(in) << " [label=\"" << getNodeCaption(in) << "\"" 
        << getInNodeShape(in) << "];\n";
  _file << "  }\n";

  _nodes.insert(getNodeName(in));
}

void GraphExporter::addOutNode(const Value& out) {
  if (_nodes.count(getNodeName(out)))
    return;

  _file << "  { rank=sink;\n";
  _file << "    " << getNodeName(out) << " [label=\"" << getNodeCaption(out) << "\"" 
        << getOutNodeShape(out) << "];\n";
  _file << "  }\n";

  _nodes.insert(getNodeName(out));
}

void GraphExporter::addBlockNode(const Value& b) {
  if (_nodes.count(getNodeName(b)))
    return;

  _file << "    " << getNodeName(b) << " [label=\"" << getNodeCaption(b) << "\"" 
        << ", weight=3, fillcolor=white];\n";

  _nodes.insert(getNodeName(b));
}

void GraphExporter::addCallNode(const Value& f, const CallInst& callInst) {
  if (_nodes.count(getNodeName(f, callInst)))
    return;

  _file << "    " << getNodeName(f, callInst) << " [label=\"" << getCallNodeCaption(f, callInst) << "\"" 
        << ", weight=3, shape=record, style=filled, fillcolor=lavender];\n";

  _nodes.insert(getNodeName(f, callInst));
}

void GraphExporter::addRelationFromCall(const Value& from, const Value& to, const CallInst& callInst, std::string reason) {
  if (_nodes.find(getNodeName(from, callInst)) == _nodes.end()) {
    _file << getNodeName(from, callInst) << " [label=\"" << getNodeCaption(from) << "\", " << getShape(from) << "];\n";
    _nodes.insert(getNodeName(from, callInst));
  }

  if (_nodes.find(getNodeName(to)) == _nodes.end()) {
    _file << getNodeName(to) << " [label=\"" << getNodeCaption(to) << "\", " << getShape(to) << "];\n";
    _nodes.insert(getNodeName(to));
  }

  if (_pairs.find(std::make_pair(getNodeName(from, callInst), getNodeName(to))) == _pairs.end()) {
    _file << getNodeName(from, callInst) << " -> " << getNodeName(to) 
          << " [label=\"" << getLabel(reason) << "\", "
          << "style=\"" << getLineStyle(reason) << "\""
          <<  "];\n";

    _pairs.insert(std::make_pair(getNodeName(from, callInst), getNodeName(to)));
  }
}

void GraphExporter::addCallAlias(const Value& alias, const CallInst& callInst) {
  addCallNode(alias, callInst);
  const Value& callee = *callInst.getCalledValue();

  if (_pairs.find(std::make_pair(getNodeName(alias, callInst), getNodeName(callee))) == _pairs.end()) {
    _file << getNodeName(alias, callInst) << " -> " << getNodeName(callee) 
          << " [label=\"function pointer\", "
          << "style=\"dashed\""
          <<  "];\n";

    _pairs.insert(std::make_pair(getNodeName(alias, callInst), getNodeName(callee)));
  }
}

void GraphExporter::addRelationToCall(const Value& from, const Value& to, const CallInst& callInst, std::string reason) {
  if (_nodes.find(getNodeName(from)) == _nodes.end()) {
    _file << getNodeName(from) << " [label=\"" << getNodeCaption(from) << "\", " << getShape(from) << "];\n";
    _nodes.insert(getNodeName(from));
  }

  if (_nodes.find(getNodeName(to, callInst)) == _nodes.end()) {
    _file << getNodeName(to, callInst) << " [label=\"" << getNodeCaption(to) << "\", " << getShape(to) << "];\n";
    _nodes.insert(getNodeName(to, callInst));
  }

  if (_pairs.find(std::make_pair(getNodeName(from), getNodeName(to, callInst))) == _pairs.end()) {
    _file << getNodeName(from) << " -> " << getNodeName(to, callInst) 
          << " [label=\"" << getLabel(reason) << "\", "
          << "style=\"" << getLineStyle(reason) << "\""
          <<  "];\n";

    _pairs.insert(std::make_pair(getNodeName(from), getNodeName(to, callInst)));
  }
}

void GraphExporter::addRelation(const Value& from, const Value& to, std::string reason) {
  if (_nodes.find(getNodeName(from)) == _nodes.end()) {
    _file << getNodeName(from) << " [label=\"" << getNodeCaption(from) << "\", " << getShape(from) << "];\n";
    _nodes.insert(getNodeName(from));
  }

  if (_nodes.find(getNodeName(to)) == _nodes.end()) {
    _file << getNodeName(to) << " [label=\"" << getNodeCaption(to) << "\", " << getShape(to) << "];\n";
    _nodes.insert(getNodeName(to));
  }

  if (_pairs.find(std::make_pair(getNodeName(from), getNodeName(to))) == _pairs.end()) {
    _file << getNodeName(from) << " -> " << getNodeName(to) 
          << " [label=\"" << getLabel(reason) << "\", "
          << "style=\"" << getLineStyle(reason) << "\""
          <<  "];\n";

    _pairs.insert(std::make_pair(getNodeName(from), getNodeName(to)));
  }
}

std::string GraphExporter::getLabel(std::string reason) const {
  return reason == "block-taint" ? "" : reason;
}

std::string GraphExporter::getLineStyle(std::string reason) const {
  return reason == "block-taint" ? "dotted" :
         reason == "function indirection" ? "dashed" : "solid";
}

std::string GraphExporter::getShape(const Value& v) const {
  return "shape=record, fillcolor=white";
}

std::string GraphExporter::getInOutNodeShape(const Value& v) const {
  return "shape=record, style=filled, fillcolor=yellow";
}

std::string GraphExporter::getInNodeShape(const Value& v) const {
  return isa<BasicBlock>(v) 
      ? "shape=record, style=filled, fillcolor=burlywood" 
      : "shape=record, style=filled, fillcolor=lightblue";
}

std::string GraphExporter::getOutNodeShape(const Value& v) const {
  return "shape=record, style=filled, fillcolor=pink";
}

std::string GraphExporter::getNodeName(const Value& i) const {
  std::stringstream ss;
  ss << "_" << (long)(&i);
  return ss.str();
}

std::string GraphExporter::getNodeName(const Value& i, const Value& j) const {
  std::stringstream ss;
  ss << getNodeName(i) << getNodeName(j);
  return ss.str();
}

std::string GraphExporter::getCallNodeCaption(const Value& v, const CallInst& callInst) const {
  std::stringstream ss;
  ss << v.getName().str() << "() | " << MetadataHelper::getFileAndLineNumber(callInst).str();
  return ss.str();
}

std::string GraphExporter::getNodeCaption(const Value& v) const {
  std::stringstream ss;

  if (!v.hasName()) {
    if (isa<SwitchInst>(v)) {
      ss << "switch " << getNodeName(v);
    } else if (isa<BranchInst>(v)) {
      ss << "br " << getNodeName(v);
    } else {
      std::string s = v.getName().str();
      replace( s.begin(), s.end(), '[', '(');
      replace( s.begin(), s.end(), ']', ')');
      replace( s.begin(), s.end(), '\n', ' ');
      ss << s << " " << getNodeName(v);
    }
  } else {
    ss << v.getName().str();
  }

  if (const Instruction* inst = dyn_cast<Instruction>(&v)) {
    if (MetadataHelper::hasMetadata(*inst))
      ss << " | " << MetadataHelper::getFileAndLineNumber(*inst).str();
  }

  return ss.str();
}

void GraphExporter::addCGFunction(const Function& f) {
  _file << "    " << getNodeName(f) << " [label=\"" << getNodeCaption(f) << "\"" 
        << ", weight=3];\n";
}

void GraphExporter::addCGCall(const Function& from, const Function& to) {
    _file << getNodeName(from) << " -> " << getNodeName(to) 
          <<  ";\n";
}

