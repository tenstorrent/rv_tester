#include <iostream>
#include <fstream>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <atomic>
#include <vector>
#include <thread>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <string>
#include "whisper_client_shm.h"

// Connect to the whisper process defined by the given file.  File
// should contain a single line consisting of a host name followed by
// a port number. Example:  good-host 1700
// Return non-negative socket number on success and -1 on failure.
int
whisperClientShm::whisperConnect(const char* filePath)
{
  path = "/" + std::string(filePath);
  fd = shm_open(path.c_str(), O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd < 0)
    {
      std::cerr << "Failed to open shared memory file\n";
      return -1;
    }
  if (ftruncate(fd, 4096) < 0)
    {
      std::cerr << "Failed ftruncate on shared memory file\n";
      return -1;
    }

  shm = (char*) mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (shm == MAP_FAILED)
    {
      std::cerr << "Failed mmap\n";
      return -1;
    }

  // claim region for client
  std::atomic_char* guard = (std::atomic_char*) shm;
  std::atomic_store(guard, 'c');
  return 1;
}


void
whisperClientShm::whisperDisconnect()
{
  if (shm)
    munmap(shm, 4096);

  if (fd > 0)
    close(fd);

  fd = -1;
  shm = nullptr;
  path = "";
}


bool
whisperClientShm::receiveMessage(char* shm, WhisperMessage& msg)
{
  // reserve first byte for locking
  std::atomic_char* guard = (std::atomic_char*) shm;
  while (std::atomic_load_explicit(guard, std::memory_order_acquire) != 'c');
  char* buffer = shm + (sizeof(uint32_t) - (reinterpret_cast<uintptr_t>(shm) % sizeof(uint32_t)));
  msg = WhisperMessage::deserializeFrom(std::span(buffer, sizeof(msg)));
  return true;
}


bool
whisperClientShm::sendMessage(char* shm, const WhisperMessage& msg)
{
  // reserve first byte for locking
  std::atomic_char* guard = (std::atomic_char*) shm;
  // while (std::atomic_load_explicit(guard, std::memory_order_acquire) != 'c');
  char* buffer = shm + (sizeof(uint32_t) - (reinterpret_cast<uintptr_t>(shm) % sizeof(uint32_t)));
  msg.serializeTo(std::span(buffer, sizeof(msg)));
  std::atomic_store_explicit(guard, 's', std::memory_order_release);
  return true;
}


bool
whisperClientShm::whisperCommand(const WhisperMessage& req, WhisperMessage& reply)
{
  if (not sendMessage(shm, req))
    {
      std::cerr << "Failed to send request to whisper\n";
      return false;
    }
  if (not receiveMessage(shm, reply))
    {
      std::cerr << "Failed to receive reply from whisper\n";
      return false;
    }
  return true;
}


bool
whisperClientShm::whisperPeek(int hart, char resource, uint64_t addr, uint64_t& value,
	    bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Peek;
  req.resource = resource;
  req.address = addr;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  value = reply.value;
  return true;
}


// Send a whisper poke command. Retrun true on successful comunication
// and false on failure. Set valid to false if hart/resource/addr
// are invalid.
bool
whisperClientShm::whisperPoke(int hart, uint64_t time, char resource, uint64_t addr, uint64_t value,
	    bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Poke;
  req.resource = resource;
  req.address = addr;
  req.value = value;
  req.time = time;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}


bool
whisperClientShm::whisperStep(int hart, uint64_t time, uint64_t instrTag, uint64_t& pc,
	    uint32_t& instruction, unsigned& changeCount,
	    std::string& disasm, uint32_t& privMode,
	    uint32_t& fpFlags, bool& hasTrap, bool& hasStop)
{
  req.hart = hart;
  req.type = WhisperMessageType::Step;
  req.instrTag = instrTag;
  req.time = time;

  if (not whisperCommand(req, reply))
    return false;

  pc = reply.address;
  instruction = reply.resource;
  changeCount = reply.value;

  // Recover privilege mode (2 bits), fpFlags (4 bits), and trap (1
  // bit) from flags field.
  unsigned mode = reply.flags & 3;
  unsigned flags = (reply.flags >> 2) & 0xf;
  unsigned trap = (reply.flags >> 6) & 1;
  unsigned stop = (reply.flags >> 7) & 1;

  privMode = mode;
  fpFlags = flags;
  hasTrap = trap;
  hasStop = stop;
  reply.buffer[reply.buffer.size() - 1] = '\0';
  disasm = reply.buffer.data();

  return true;
}

// Copied from chuang's LSTB Whisper Client
bool
whisperClientShm::whisperSimpleStep(int hart, uint64_t& pc, uint32_t& instruction, unsigned& changeCount)
{
  req.hart = hart;
  req.type = WhisperMessageType::Step;
  req.instrTag = 0;
  req.time = 0;

  WhisperMessage reply;
  if (not whisperCommand(req, reply))
    return false;

  pc = reply.address;
  instruction = reply.resource;
  changeCount = reply.value;

  return true;
}

bool
whisperClientShm::whisperChange(int hart, uint32_t& resource, uint64_t& addr, uint64_t& value,
	      bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Change;
  if (not whisperCommand(req, reply))
    return false;

  resource = reply.resource;
  addr = reply.address;
  value = reply.value;
  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

//void
//whisperClientShm::whisperChanges(int hart, std::unordered_map<uint32_t,uint64_t>& addrs, std::unordered_map<uint32_t,uint64_t>& datas,
//               int changeCount)
//{
//  for (int i = 0; i < changeCount; i++)
//    {
//      uint32_t res;
//      uint64_t addr, value;
//      bool valid;
//      assert(whisperChange(hart, res, addr, value, valid) and valid);
//      addrs[res] = addr;
//      datas[res] = value;
//    }
//}

bool
whisperClientShm::whisperMcmRead(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
	       unsigned size, uint64_t value, bool internal, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmRead;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr;
  req.value = value;
  req.size = size;
  req.flags = internal;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

bool
whisperClientShm::whisperMcmInsert(int hart, uint64_t time, uint64_t instrTag, uint64_t addr,
		 unsigned size, uint64_t value, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmInsert;
  req.time = time;
  req.instrTag = instrTag;
  req.address = addr;
  req.value = value;
  req.size = size;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

bool
whisperClientShm::whisperMcmWrite(int hart, uint64_t time, uint64_t addr,
		unsigned size, svOpenArrayHandle handle, uint64_t mask, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::McmWrite;
  req.time = time;
  req.address = addr;
  req.size = size;
  req.flags = 1;
  for (uint8_t i = 0; i < req.size/8; ++i) {
    req.tag[i] = (uint8_t)((mask >> (i*8)) & 0xff);
  }
  if (req.size > req.buffer.size())
    {
      std::cerr << "whisperMcmWrite: write size too large: " << req.size << '\n';
      valid = false;
      return true;
    }

  uint8_t* data = reinterpret_cast<uint8_t*> (handle);
  for (unsigned i = 0; i < size; ++i)
    req.buffer[i] = data[i];

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

bool
whisperClientShm::whisperTranslate(int hart, uint64_t vaddr, bool r, bool w, bool x,
         bool supervisor, uint64_t& paddr, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Translate;
  req.address = vaddr;
  req.flags = 0;

  if      (r) req.flags |= 1;
  else if (w) req.flags |= 2;
  else if (x) req.flags |= 4;

  if (supervisor) req.flags |= 8;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  if (valid)
    paddr = reply.address;
  return true;
}

//bool
//whisperClientShm::whisperCancelLr(int hart, bool& valid)
//{
//  req.hart = hart;
//  req.type = WhisperMessageType::CancelLr;
//
//  if (not whisperCommand(req, reply))
//    return false;
//
//  valid = reply.type != WhisperMessageType::Invalid;
//  return true;
//}

bool
whisperClientShm::whisperReset(int hart, bool& valid)
{
  req.hart = hart;
  req.type = WhisperMessageType::Reset;

  if (not whisperCommand(req, reply))
    return false;

  valid = reply.type != WhisperMessageType::Invalid;
  return true;
}

bool
whisperClientShm::whisperQuit()
{
  WhisperMessage req(0 /*hart*/, WhisperMessageType::Quit);  // Any hart will do
  if (not sendMessage(shm, req))
    {
      std::cerr << "Failed to send request to whisper\n";
      return false;
    }

  whisperDisconnect();

  // There is no reply for quit command.
  return true;
}

//bool
//whisperClientShm::whisperQuit()
//{
//  req.hart = 0;
//  req.type = WhisperMessageType::Quit;
//
//  if (not whisperCommand(req, reply))
//    {
//      std::cerr << "Failed to send request to whisper\n";
//      return false;
//    }
//
//  whisperDisconnect();
//
//  // There is no reply for quit command.
//  return true;
//}

bool
whisperClientShm::whisperPageTableWalk(int hart, bool isInstr, bool isAddr,
		     svOpenArrayHandle items, unsigned& itemCount,
		     bool& valid)
{
  return true;
}

bool
whisperClientShm::whisperEnterDebug()
{
  req.hart = 0;
  req.type = WhisperMessageType::EnterDebug;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

bool
whisperClientShm::whisperExitDebug()
{
  req.hart = 0;
  req.type = WhisperMessageType::ExitDebug;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

// Send a whisper check_interrupt command. Return true on successful communication
// and false on failure. Set interrupt to true/false if interrupt is/is-not
// possible assuming the MIP CSR has the given mip value.
bool
whisperClientShm::whisperCheckInterrupt(int hart, uint64_t mip, bool& interrupt)
{
  req.hart = hart;
  req.type = WhisperMessageType::CheckInterrupt;
  req.address = mip;

  WhisperMessage reply;

  if (not whisperCommand(req, reply))
    return false;

  interrupt = reply.flags;
  return true;
}


// Set the supervisor mode external interrupt pin to the given
// value (0 or 1). Whisper will consider either the SEIP bit in MIP or the
// the external pin value set by this method for taking a supervisor
// external interrupt (assuming that interrupt is enabled).
bool
whisperClientShm::whisperSetSeiPin(int hart, uint64_t value)
{
  req.hart = hart;
  req.type = WhisperMessageType::SeiPin;
  req.value = value;

  WhisperMessage reply;

  if (not whisperCommand(req, reply))
    return false;

  return true;
}

#if 0

int
main(int argc, char* argv[])
{
  if (argc == 2)
    {
      std::string path = argv[1];
      int soc = whisperConnect(path);
      if (soc >= 0)
	{
	  std::cerr << "Connected to socket: " << soc << '\n';
	  uint64_t value = 0;
	  unsigned hart = 0;
	  bool valid = false;
	  whisperPeek(soc, hart, 'r', 1, value, valid);
	  whisperPeek(soc, hart, 'r', 2, value, valid);
	  whisperPoke(soc, hart, 'p', 0, 0x1234, valid);
	  whisperPeek(soc, hart, 'p', 0, value, valid);

	  unsigned count = 0;
	  whisperStep(soc, hart, count);
	  std::cerr << "Change count: " << std::hex << count << '\n';
	  for (unsigned i = 0; i < count; ++i)
	    {
	      unsigned resource = 0;
	      uint64_t addr = 0;
	      if (whisperChange(soc, hart, resource, addr, value, valid) and valid)
		std::cerr << "  " << char(resource) << "  " << std::hex << addr
			  << ' ' << std::hex << value << '\n';
	    }
	  whisperQuit(soc);
	}
    }
  return 0;
}

#endif
