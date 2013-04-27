#include "TaintFile.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <llvm/Instructions.h>

/**
 * Reads the taint file for the given function and 
 * converts the mapping into a numeric form of
 * 0 => 1, 2 => -1, ...
 * where the numbers specify the argument position
 * or -1 for return-value and -2 for varargs.
 *
 * The created mapping can be retreived via getMapping()
 */
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
  FunctionTaintMap& mapping = taints->getMapping();

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

    // Convert left hand side of => 
    //

    stringstream convert1(paramName);
    // If integer conversion failed, the arguments
    // in the file are specified by their names so we 
    // have to search the corresponing argument positions.
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

    // Convert right hand side of => 
    //

    i = 0;
    stringstream convert2(valName);
    // If integer conversion failed, the arguments
    // in the file are specified by their names so we 
    // have to search the corresponing argument positions.
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

      if (valName.compare("...") == 0) {
        retvalPos = -2;
        DEBUG(debugStream << "Interpreting as varargs\n");
      }
    } else {
      DEBUG(debugStream << "Retval-info from file: seem to be at #" << retvalPos << "\n");
    }

    if (paramPos == -1) {
      DEBUG(debugStream << "  - Skipping `" << paramName << "` -- not found.\n");
      break;
    }

    DEBUG(debugStream << " Insert mapping: " << paramPos << " => " << retvalPos << "\n");
    mapping.insert(make_pair(paramPos, retvalPos));
  }

  file.close();

  DEBUG(debugStream << " Inserted " << mapping.size() << " mappings\n");

  return taints;
}

/**
 * @return true if the taint file for the given function exists,
 * false otherwise
 */
bool TaintFile::exists(const Function& f) {
  ifstream file(getFilename(f).c_str());
  return file.good();
}

/**
 * Removes the taint file for the specified function
 */
void TaintFile::remove(const Function& f) {
  ::remove(getFilename(f).c_str());
}

/**
 * Extract the taint file name from the given Function
 */
string TaintFile::getFilename(const Function& f) {
  return f.getName().str() + ".taints";
}

/**
 * Writes the provided ResultSet to a taint file
 * for the provided Function
 */
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
    else
      // Varargs
      sinkPos = -2;

      file << sourcePos << " => " << sinkPos << "\n"; 
  }

  file.close();
}
