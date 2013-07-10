#include "TaintFile.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include "llvm/IR/Instructions.h"

map<const Function*, FunctionTaintMap> TaintFile::_mappingCache;

/**
 * Taint mappings are read from file and cached for further use.
 * The cache is static and so it is valid for the whole analysis,
 * eg. every file is only read once.
 * This is ok, because mappings usually don't change. (Exception:
 * mutual recursive calls -- but the handling ensures the old 
 * (partial) file gets deleted and the cache entry is removed, 
 * forcing a re-read of the mapping.
 */
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

  IF_PROFILING(logger.profile() << "Getting mapping from cache for `" << func.getName() << "` took "
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
      file.open((filename + ".temp").c_str(), ios::in);

      if (!file.is_open()) {
        logger.debug() << " -- Cannot get information about `" << func.getName() << "` -- cancel.\n";
        return false;
      }
    }
  }


  string line;
  while (file.good()) {
    getline(file, line);
    
    string sourceName;
    string sinkName;
    istringstream iss(line);

    iss >> sourceName;
    // consume => :
    iss >> sinkName;
    iss >> sinkName;

    if (sourceName.size() == 0 || sinkName.size() == 0) {
      continue;
    }

    // Convert names to positions
    int sourcePos = getValuePosition(func, logger, sourceName);
    int sinkPos = getValuePosition(func, logger, sinkName);

    DEBUG(logger.debug() << "Insert mapping: " << sourcePos << " => " << sinkPos << " ");
    DEBUG(logger.debug() << "with names: " << sourceName << " => " << sinkName << "\n");
    mapping.insert(FunctionTaint(sourceName, sourcePos, sinkName, sinkPos));
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

    if (!file.is_open()) {
      file.open((filename + ".temp").c_str(), ios::in);

      if (!file.is_open())
        return false;
    }
  }
  
  return file.good();
}

int TaintFile::getValuePosition(const Function& func, const Logger& logger, const string valName) {
    int i = 0;
    int position;

    stringstream convert(valName);

    if(convert >> position) {
      DEBUG(logger.debug() << "Using value position from file: `" << valName << "` is at #" << position << "\n");
      return position;
    }
    
    DEBUG(logger.debug() << "Searching for value `" << valName << "`\n");

    // If integer conversion failed, the arguments
    // in the file are specified by their names so we 
    // have to search the corresponing argument positions.

    // Check for known special names first before scanning
    // the argument list
    if (valName.compare("...") == 0) {
      position = -2;
      DEBUG(logger.debug() << " Interpreting as varargs\n");
    } else if (valName.compare("$_retval") == 0) {
      position = -1;
      DEBUG(logger.debug() << " Interpreting as return value\n");
    } else if (valName.compare(0, 1, "@") == 0) {
      position = -3;
      DEBUG(logger.debug() << " Interpreting as global value\n");
    } else {
      position = -3;

      for (Function::const_arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; ++a_i) {
        if (valName.compare(a_i->getName().str()) == 0) {
          position = i;
          DEBUG(logger.debug() << " Found at #" << i << "\n");
          break;
        }

        i++;
      }
    }

    return position;
}

/**
 * Removes the taint file for the specified function
 */
void TaintFile::remove(const Function& f) {
  ::remove((getFilename(f) + ".temp").c_str());
  _mappingCache.erase(&f);
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
void TaintFile::writeTempResult(const Function& f, const ResultSet result) {
  ofstream file;

  file.open((getFilename(f) + ".temp").c_str(), ios::out);

  for (ResultSet::const_iterator i = result.begin(), e = result.end(); i != e; ++i) {
    const Value& arg = *i->first;
    const Value& retval = *i->second;

    string source = Helper::getValueName(arg);
    string sink = Helper::getValueName(retval);

    file << source << " => " << sink << "\n";

    // Specify the taint a second time in numeric form, eg 0 => -1 
    // (except for globals, they keep their name)

    if (isa<Argument>(arg)) {
      ostringstream convert;
      convert << cast<Argument>(arg).getArgNo(); 
      source = convert.str();
    } else if (!isa<GlobalValue>(arg)) {
      // Varargs
      source = "-2";
    }

    if (isa<ReturnInst>(retval)) {
      sink = "-1";
    } else if (isa<Argument>(retval)) {
      ostringstream convert;
      convert << cast<Argument>(retval).getArgNo();
      sink = convert.str();
    } else if (!isa<GlobalValue>(retval)) {
      // Varargs
      sink = "-2";
    }

    file << source << " => " << sink << "\n"; 
  }

  file.close();

  // Remove from cache because the mapping changed
  _mappingCache.erase(&f);
}

void TaintFile::persistResult(const Function& f) {
  ::rename((getFilename(f) + ".temp").c_str(), getFilename(f).c_str());
}
