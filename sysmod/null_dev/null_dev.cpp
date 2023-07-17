#include <iostream>
#include "null_dev.h"


void null_dev::write(const transactor::write_t&) {
  return;
}

void null_dev::read(const transactor::read_t&, data_t&) {
  return;
}


// bool null_dev::init_elf(const std::string& path) {
//   std::cout<<"[null_dev]: Device init elf\n";
//     try {
//         m_.load_ELF(path);
//     } catch(const std::exception& e) {
//         std::cerr << e.what() << "\n";
//         return false;
//     }
//     return true;
// }
