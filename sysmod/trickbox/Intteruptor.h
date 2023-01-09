void trickbox::handle_itp(uint64_t offset,uint64_t dword,cbs_t& cbs){
  
  if(offset == 0){
   //immidiate interrupt
  unsigned hart = dword & 0xfff;
  unsigned  event = (dword >> 12) & 0xff;
  unsigned eventValue = (dword >> 20);
  bool flag = eventValue != 0;
  trickbox::trickboxInterrupt(hart, flag, event, cbs);
  }else if(offset < 1000){
  //delayed interrupt
  //unsigned hart = dword & 0xfff;
  //unsigned  event = (dword >> 12) & 0xff;
  //unsigned eventValue = (dword >> 20);
  //unsigned flag = eventValue != 0;
  //delayed interrupts
    unsigned event = (addr>> 7 )& 0xf; //[10:7];//(value >> 12) & 0xff;
    unsigned eventValue_delay = (dword>>12) & 0x7ffff;//[30:0] ;
    unsigned flag_m = dword & 0x80000000;
    bool flag = flag_m !=0;
    unsigned hart = dword & 0xfff; //
  trickbox::trickboxDelayedInterrupt(hart, flag, event,eventValue_delay, cbs);
  }


}
