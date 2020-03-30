#include "SplitString.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
 
std::vector<std::string> SplitString(const std::string &base, const char *sep) {
    std::vector<std::string> result;

    for (size_t pos = 0; pos < base.size(); ) {
        size_t delimiter = base.find_first_of(sep, pos);
        if (delimiter == pos) {
            // we have an empty entry, we skip
        } else if (delimiter == std::string::npos) {
            // No delimiter until the end of the string
            result.push_back(base.substr(pos));
            break;
        } else {
            result.push_back(base.substr(pos, delimiter - pos));
        }
        pos = delimiter + 1;
    }
    return result;
}

