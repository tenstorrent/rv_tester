#pragma once

#include <string>

#include "cvm/plusargs.hpp"
#include "cvm/logger.hpp"

#include "util.h"
#include "arch_sample.h"

class bot {

  public:

    bot();

  private:
    
    void run_iss_standalone();
    ArchSample archcov;
};
