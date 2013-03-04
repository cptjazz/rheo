#include <string>
#include <set>
#include <llvm/Value.h>

using namespace std;
using namespace llvm;

class Helper {
public:
  static string getValueNameOrDefault(Value& v);   
  static bool areSetsEqual(set<Value*>& s1, set<Value*>& s2);
};
