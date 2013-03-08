#include "TaintFile.h"
#include <iostream>
#include <fstream>
#include <sstream>

TaintFile* TaintFile::read(Function& func, raw_ostream& debugStream) {
  string filename = func.getName().str() + ".taints";
  ifstream file(filename.c_str(), ios::in);

  if (!file.is_open()) {
    file.open(("taintlib/" + filename).c_str(), ios::in);

    if (!file.is_open()) {
      debugStream << " -- Cannot get information about `" << func.getName() << "` -- cancel.\n";
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
      debugStream << "Searching for param " << paramName << "\n";
      for (Function::arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (a_i->getName().str() == paramName) {
          paramPos = i;
          debugStream << "Found at #" << i << "\n";
          break;
        }

        i++;
      }
    } else {
      debugStream << "Param-info from file: seem to be at #" << paramPos << "\n";
    }

    i = 0;
    stringstream convert2(valName);
    if( !(convert2 >> retvalPos)) {
      retvalPos = -1;
      debugStream << "Searching for retval " << valName << "\n";
      for (Function::arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (a_i->getName().str() == valName) {
          retvalPos = i;
          debugStream << "Found at #" << i << "\n";
          break;
        }

        i++;
      }
    } else {
      debugStream << "Retval-info from file: seem to be at #" << retvalPos << "\n";
    }

    if (paramPos == -1) {
      debugStream << "  - Skipping `" << paramName << "` -- not found.\n";
      delete(taints);
      return NULL;
    }

    FunctionTaintMap& mapping = taints->getMapping();
    debugStream << " Insert mapping: " << paramPos << " => " << retvalPos << "\n";
    mapping.insert(make_pair(paramPos, retvalPos));
  }

  file.close();

  return taints;
}

bool TaintFile::exists(Function& f) {
  ifstream file(f.getName().str().c_str());
  return file.good();
}

void TaintFile::writeResult(Function& f, ResultSet result) {
  ofstream file;
  file.open((f.getName().str() + ".taints").c_str(), ios::out);

  for (ResultSet::iterator i = result.begin(), e = result.end(); i != e; ++i) {
    Value& arg = *i->first;
    Value& retval = *i->second;

    file << arg.getName().str() << " => " << Helper::getValueNameOrDefault(retval) << "\n";
  }

  file.close();
}
