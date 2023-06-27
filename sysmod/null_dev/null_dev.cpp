#include <iostream>
#include "null_dev.h"


 void null_dev::write(uint64_t addr, size_t, const data_t&, const strb_t&) {
   if (not has_addr(addr))
     return;
   return;
 }

cvm::messenger::task<void> null_dev::read(uint64_t addr, size_t, data_t&) {
   co_return;
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
