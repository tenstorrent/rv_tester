#include <iostream>
#include "null_dev.h"


 void null_dev::write(uint64_t addr, size_t length, const data_t& data, const strb_t& strb,
                 cbs_t& cbs) {
   if (not has_addr(addr))
     return;
   return;
 }

 void null_dev::read(uint64_t addr, size_t length, data_t& data, cbs_t& cbs) {
   if (not has_addr(addr))
     return;


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
