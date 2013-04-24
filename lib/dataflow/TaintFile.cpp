#include "TaintFile.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <llvm/Instructions.h>

TaintFile* TaintFile::read(const Function& func, raw_ostream& debugStream) {
  string filename = getFilename(func);
  ifstream file(filename.c_str(), ios::in);

  if (!file.is_open()) {
    file.open(("taintlib/" + filename).c_str(), ios::in);

    if (!file.is_open()) {
      DEBUG(debugStream << " -- Cannot get information about `" << func.getName() << "` -- cancel.\n");
      return NULL;
    }
  }

  TaintFile* taints = new TaintFile();

  string line;
  while (file.good()) {
    getline(file, line);
    
    string paramName;
    string valName;
    istringstream iss(line);

    iss >> paramName;
    // consume => :
    iss >> valName;
    iss >> valName;

    int paramPos;
    int retvalPos;
    int i = 0;

    if (paramName.size() == 0 || valName.size() == 0) {
      continue;
    }

    stringstream convert1(paramName);
    if( !(convert1 >> paramPos)) {
      paramPos = -1;
      DEBUG(debugStream << "Searching for param " << paramName << "\n");

      for (Function::const_arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (a_i->getName().str() == paramName) {
          paramPos = i;
          DEBUG(debugStream << "Found at #" << i << "\n");
          break;
        }

        i++;
      }

      if (paramName.compare("...") == 0)
        paramPos = -2;

    } else {
      DEBUG(debugStream << "Param-info from file: seem to be at #" << paramPos << "\n");
    }

    i = 0;
    stringstream convert2(valName);
    if( !(convert2 >> retvalPos)) {
      retvalPos = -1;
      DEBUG(debugStream << "Searching for retval " << valName << "\n");
      for (Function::const_arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (a_i->getName().str() == valName) {
          retvalPos = i;
          DEBUG(debugStream << "Found at #" << i << "\n");
          break;
        }

        i++;
      }

      if (paramName.compare("...") == 0)
        paramPos = -2;

    } else {
      DEBUG(debugStream << "Retval-info from file: seem to be at #" << retvalPos << "\n");
    }

    if (paramPos == -1) {
      DEBUG(debugStream << "  - Skipping `" << paramName << "` -- not found.\n");
      break;
    }

    FunctionTaintMap& mapping = taints->getMapping();
    DEBUG(debugStream << " Insert mapping: " << paramPos << " => " << retvalPos << "\n");
    mapping.insert(make_pair(paramPos, retvalPos));
  }

  file.close();

  return taints;
}

bool TaintFile::exists(const Function& f) {
  ifstream file(getFilename(f).c_str());
  return file.good();
}

void TaintFile::remove(const Function& f) {
  ::remove(getFilename(f).c_str());
}

string TaintFile::getFilename(const Function& f) {
  return f.getName().str() + ".taints";
}

void TaintFile::writeResult(const Function& f, const ResultSet result) {
  ofstream file;
  file.open((f.getName().str() + ".taints").c_str(), ios::out);

  for (ResultSet::const_iterator i = result.begin(), e = result.end(); i != e; ++i) {
    const Value& arg = *i->first;
    const Value& retval = *i->second;

    int sourcePos = -3;
    int sinkPos = -3;

    file << arg.getName().str() << " => " << Helper::getValueNameOrDefault(retval) << "\n";

    // Specify the taint a second time in numeric form, eg 0 => -1
    if (isa<Argument>(arg))
      sourcePos = cast<Argument>(arg).getArgNo(); 
    else if (!isa<GlobalVariable>(arg))
      // Varargs
      sourcePos = -2;

    if (isa<ReturnInst>(retval))
      sinkPos = -1;
    else if (isa<Argument>(retval))
      sinkPos = cast<Argument>(retval).getArgNo();

      file << sourcePos << " => " << sinkPos << "\n"; 
  }

  file.close();
}
