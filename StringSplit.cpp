#include <sstream>
#include "StringSplit.h"

std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    auto elems = std::vector<std::string>();
    auto item = std::string();

    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }

    return elems;
}


