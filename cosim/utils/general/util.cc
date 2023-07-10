#include "util.h"

#include <cstdio>
#include <iostream>
#include <sstream>          // stringstream
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <algorithm>


// https://stackoverflow.com/a/478960
std::string cosim_util::exec(const char* cmd)
 {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// https://stackoverflow.com/a/217605
// trim from end (in place)
void cosim_util::rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
                }).base(), s.end());
}

std::string cosim_util::get_nth_word(const std::string& s, int n) {
    std::istringstream iss(s);
    std::string word;
    for (int i = 0; i < n; i++) {
        if (!(iss >> word))
            return ""; // Return an empty string if there are fewer than 3 words
    }
    // Remove trailing comma if present
    if (!word.empty() && word.back() == ',') {
        word.pop_back();
    }
    return word;
}
