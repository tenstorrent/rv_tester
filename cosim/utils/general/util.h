#include <string>

namespace cosim_util {
    std::string exec(const char* cmd);
    void rtrim(std::string &s);
    std::string get_nth_word(const std::string& s, int n);
};
