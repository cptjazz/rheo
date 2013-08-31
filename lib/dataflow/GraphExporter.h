#ifndef GRAPHEXPORTER_H
#define GRAPHEXPORTER_H

#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include "Core.h"


class GraphExporter {

public:
  GraphExporter(std::string functionName) : _functionName(functionName) { }

  ~GraphExporter();

  void addInOutNode(const Value& inout);
  void addInNode(const Value& in);
  void addOutNode(const Value& out);
  void addBlockNode(const Value& b);
  void addCallNode(const Value& f, const CallInst& callInst);
  void addRelationToCall(const Value& from, const Value& to, const CallInst& callInst, std::string reason);
  void addRelationFromCall(const Value& from, const Value& to, const CallInst& callInst, std::string reason);
  void addCallAlias(const Value& alias, const CallInst& callInst);
  void addRelation(const Value& from, const Value& to, std::string reason = "");

  void addCGFunction(const Function& f);
  void addCGCall(const Function& from, const Function& to);
    
  void init();

protected:
  GraphExporter() { }

private:
  const std::string _functionName;
  std::ofstream _file;
  std::set<std::pair<std::string, std::string> > _pairs;
  std::set<std::string> _nodes;

  std::string getShape(const Value& v) const;
  std::string getInOutNodeShape(const Value& v) const;
  std::string getInNodeShape(const Value& v) const;
  std::string getOutNodeShape(const Value& v) const;
  std::string getNodeName(const Value& i) const;
  std::string getNodeName(const Value& i, const Value& j) const;
  std::string getNodeCaption(const Value& v) const;
  std::string getCallNodeCaption(const Value& v, const CallInst& callInst) const;
  std::string getLineStyle(std::string reason) const;
  std::string getLabel(std::string reason) const;
};

#endif
