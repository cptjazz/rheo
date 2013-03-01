#include <string>
#include <llvm/Value.h>

using namespace std;
using namespace llvm;

class Helper {
public:
  static string getValueNameOrDefault(Value& v);   
};
