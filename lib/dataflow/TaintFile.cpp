#include "TaintFile.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <llvm/Instructions.h>

map<const Function*, FunctionTaintMap> TaintFile::_mappingCache;

const FunctionTaintMap* TaintFile::getMapping(const Function& func, const Logger& logger) {
  IF_PROFILING(long t = Helper::getTimestamp());
  FunctionTaintMap temp;
  pair<TaintFileCache::iterator, bool> result = _mappingCache.insert(make_pair(&func, temp));
  FunctionTaintMap& mapping = result.first->second;

  if (result.second) {
    IF_PROFILING(long t = Helper::getTimestamp());

    bool readResult = read(func, logger, mapping);
    if (!readResult)
      _mappingCache.erase(&func);

    IF_PROFILING(logger.profile() << "Reading mapping for `" << func.getName() << "`  from file took "
          << Helper::getTimestampDelta(t) << "µs\n");
  }

  IF_PROFILING(logger.profile() << "Getting mapping for `" << func.getName() << "` took "
        << Helper::getTimestampDelta(t) << "µs\n");

  return &mapping;
}

/**
 * Reads the taint file for the given function and 
 * converts the mapping into a numeric form of
 * 0 => 1, 2 => -1, ...
 * where the numbers specify the argument position
 * or -1 for return-value and -2 for varargs.
 *
 * The created mapping can be retreived via getMapping()
 */
bool TaintFile::read(const Function& func, const Logger& logger, FunctionTaintMap& mapping) {
  string filename = getFilename(func);
  ifstream file(filename.c_str(), ios::in);

  if (!file.is_open()) {
    file.open(("taintlib/" + filename).c_str(), ios::in);

    if (!file.is_open()) {
      logger.debug() << " -- Cannot get information about `" << func.getName() << "` -- cancel.\n";
      return false;
    }
  }


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
      paramPos = -3;
      DEBUG(logger.debug() << "Searching for param " << paramName << "\n");

      for (Function::const_arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (a_i->getName().str() == paramName) {
          paramPos = i;
          DEBUG(logger.debug() << " Found at #" << i << "\n");
          break;
        }

        i++;
      }

      if (paramName.compare("...") == 0)
        paramPos = -2;

    } else {
      DEBUG(logger.debug() << "Param-info from file: seem to be at #" << paramPos << "\n");
    }

    // Convert right hand side of => 
    //

    i = 0;
    stringstream convert2(valName);
    // If integer conversion failed, the arguments
    // in the file are specified by their names so we 
    // have to search the corresponing argument positions.
    if( !(convert2 >> retvalPos)) {
      retvalPos = -3;
      DEBUG(logger.debug() << "Searching for retval " << valName << "\n");
      for (Function::const_arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (a_i->getName().str() == valName) {
          retvalPos = i;
          DEBUG(logger.debug() << " Found at #" << i << "\n");
          break;
        }

        i++;
      }

      if (valName.compare("...") == 0) {
        retvalPos = -2;
        DEBUG(logger.debug() << " Interpreting as varargs\n");
      }

      if (valName.compare("$_retval") == 0) {
        retvalPos = -1;
        DEBUG(logger.debug() << " Interpreting as return value\n");
      }
    } else {
      DEBUG(logger.debug() << "Retval-info from file: seem to be at #" << retvalPos << "\n");
    }

    DEBUG(logger.debug() << "Insert mapping: " << paramPos << " => " << retvalPos << " ");
    DEBUG(logger.debug() << "with names: " << paramName << " => " << valName << "\n");
    mapping.insert(FunctionTaint(paramName, paramPos, valName, retvalPos));
  }

  file.close();

  DEBUG(logger.debug() << " * Inserted " << mapping.size() << " mappings\n");
  return true;
}

/**
 * @return true if the taint file for the given function exists,
 * false otherwise
 */
bool TaintFile::exists(const Function& f) {
  string filename = getFilename(f);
  ifstream file(filename.c_str(), ios::in);

  if (!file.is_open()) {
    file.open(("taintlib/" + filename).c_str(), ios::in);

    if (!file.is_open())
      return false;
  }
  
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

    string source = arg.getName().str();
    string sink = Helper::getValueNameOrDefault(retval);

    file << source << " => " << sink << "\n";

    // Specify the taint a second time in numeric form, eg 0 => -1 
    // (except for globals, they keep their name)
    if (isa<Argument>(arg)) {
      ostringstream convert;
      convert << cast<Argument>(arg).getArgNo(); 
      source = convert.str();
    } else if (!isa<GlobalVariable>(arg)) {
      // Varargs
      source = "-2";
    }

    if (isa<ReturnInst>(retval)) {
      sink = "-1";
    } else if (isa<Argument>(retval)) {
      ostringstream convert;
      convert << cast<Argument>(retval).getArgNo();
      sink = convert.str();
    } else if (!isa<GlobalVariable>(arg)) {
      // Varargs
      sink = "-2";
    }

    file << source << " => " << sink << "\n"; 
  }

  file.close();

  // Remove from cache because the mapping changed
  _mappingCache.erase(&f);
}
