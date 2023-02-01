#include "clint_helper.h"

DECLARE_string(load);



void clint_helper::process(const transactions::m_mcmi_store& m_mcmi_store) {
 //0x02000000 0x0000c000
      svScope scope = svGetScopeFromName("top.tester.sysmod");
      svSetScope(scope); 
 //std::cout <<"CLINT HELPER  :process() Addr:"<<std::hex<<m_mcmi_store.addr<<" data: "<<m_mcmi_store.data<<"\n";
 uint64_t addr = m_mcmi_store.addr;
 if (not has_addr(addr))
    return;

  uint64_t offset = addr - 0x2000000;
  //std::cout<<"CLINT HELPER: FIRST OFFSET "<<std::hex<<offset<<" data :"<<m_mcmi_store.data <<"\n";
  if (offset < 0x4000)
    {
      // Sofware interrupt: 1 word per hart.
      //svScope scope = svGetScopeFromName("top.tester.sysmod");
      //svSetScope(scope); 
      unsigned hartIx = offset / 4;
      if ((offset % 4) != 0  or hartIx >= hartCount_)
	return;
      unsigned word = m_mcmi_store.data;
      soft_.at(hartIx) = word & 1;
      //softwareInterrupt(hartIx, word & 1);
      //std::cout<<"\nCLINT HELPER : CALL SW ITP DPI WITH FLAG \n";
      sysmod_sw_interrupt(hartIx, word & 1);
    }

      //std::lock_guard<std::mutex> lock(mutex_);

      offset -= 0x4000;

     //std::cout<<"CLINT HELPER: SECOND OFFSET "<<std::hex<<offset<<" data: "<<m_mcmi_store.data<<"\n";
    if (offset == 0x7ff8){
          //std::cout<<"CLINT HELPER: STOPPING TIMER 1\n";
          //std::cout<<"CLINT HELPER: SECOND OFFSET  data: "<<m_mcmi_store.data<<"\n";
	       timer_ = m_mcmi_store.data;
        // if(m_mcmi_store.data == 0){
        //  sysmod_timer_interrupt(0, 0);
         //run_timer_ = 1;
        //  std::cout<<"CLINT HELPER: Deasserting Timer Interrupt\n";
        // }else{
        //  std::cout<<"CLINT HELPER: Deasserting Timer Interrupt\n";
        //  sysmod_timer_interrupt(0, 0);
        // }
	  }
    else
	  {
    //std::cout<<"CLINT HELPER: 7ff8 else\n";
	  unsigned hartIx = offset / 8;
	  if (hartIx >= hartCount_)
	    return;

	  // Time compare. 1 double word per hart.
	  uint64_t dword = m_mcmi_store.data;
	  timeCompare_.at(hartIx) = dword;
          std::cout << "CLINT HELPER: Programming timercompare with "<<dword<<"for hart "<<hartIx<<"\n";
          //run_timer_ = 0;
          sysmod_timer_interrupt(0, 0);
  	}

    //std::cout<<"CLINT HELPER: 59: processtimerItp\n";
      processTimerInterrupts();



 //////////////////
  //if (tohost_addr_ != m_mcmi_store.addr)
  //  return;

  //if (tohost_status_ != (m_mcmi_store.data & 0x1))
  //  return;

  //if (tohost_device_syscall_ != ((m_mcmi_store.data >> 56) & 0xff))
  //  return;

  //uint64_t cycle = m_mcmi_store.cycle;
  //uint64_t exit_code = (m_mcmi_store.data >> 1) & 0x7fffffffffff;
  
  //if (exit_code == 0) {
  //  cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
  //  cvm::log(cvm::NONE, "<{}> Pass condition detected - tohost[0]=1, tohost[47:1]=0\n", cycle);
  //  cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
  //  vpi_control(vpiFinish);
  //} else {
  //  cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
  //  cvm::log(cvm::NONE, "<{}> Error: Fail condition detected - tohost[0]=1, tohost[47:1]={:#x}\n", cycle, 
  //    exit_code);
  //  cvm::log(cvm::NONE, "<{}> ---------------------------------------------\n", cycle);
  //  vpi_control(vpiFinish);
  //}
}




void
clint_helper::selfTick(useconds_t delta)
{
    //std::cout<<"\nCLINT HELPER: SELF TICK \n";
  auto func = [this, delta]() {
    while (true)
      {
	usleep(delta);
  //std::cout<<"CLINT TMR SELF TICK\n";
	if (terminate_)
	  return;
	else
	  {
	    //std::lock_guard<std::mutex> lock(mutex_);
	     tick(200);
	  }
      }
  };

  timerThread_ = std::thread(func);
}

