#include <gflags/gflags.h>
#include <stdio.h>
#include <cstring>

// shared flags here?
int argc = 3;
char* argv[4];

extern "C" {
  void parse_flags(char* gflagfile) {
    argv[0] = (char*) "dummy";
    argv[1] = (char*) "--flagfile";
    argv[2] = gflagfile;
    argv[3] = NULL;

    // otherwise addresses are the same
    char** p = argv;

    gflags::ParseCommandLineFlags(&argc, &p, false);
  }
}
