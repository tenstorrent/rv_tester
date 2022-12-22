#pragma once

#include "whisper_client.h"

#include <string>
#include <iostream>         // cout
#include <thread>           // std::this_thread::sleep_for
#include <chrono>           // std::chrono::seconds
#include <cstdlib>          // system


namespace cosim {

template <typename F, typename... Args>
bool whisper_api(F func, Args&&... args) {

  if (!func(std::forward<Args>(args)...)) {
    std::cout << "Error: Whisper API call failed";
    return false;
  }

  return true;
}

// Whisper connect
bool whisper_connect_api(std::string cmd, int timeout) {
  
  std::cout << "Cosim whisper command: " << cmd << "\n";
  system(cmd.c_str());

  auto start = std::chrono::high_resolution_clock::now();
  while (true) {
    std::this_thread::sleep_for (std::chrono::milliseconds(30));
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (whisperConnect("whisper_connect") >= 0) {
      std::cout << "Whisper connect succeeded in " << std::dec << duration << " ms\n";
      return true;
    }
    else if (duration > timeout) {
      std::cout << "Error: Whisper connect failed. Stopping after " << duration << " ms.\n";
      return false;
    }
  }
}

// Whisper quit and disconnect
void whisper_quit_api() {
  std::cout << "Whisper quit...\n";
  whisperQuit();
}

}
