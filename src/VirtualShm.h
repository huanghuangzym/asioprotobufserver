#ifndef __VIRTUALSHM__HH__
#define __VIRTUALSHM__HH__
#include "MemUtil.h"

#include <Bmco/SharedPtr.h>
#include <boost/format.hpp>

namespace BM35 {
/// number of waiting seconds for time_lock, timed_sharable_lock etc.
const Bmco::UInt32	TIMED_LOCK_WAIT_SECONDS = 10;
/// milliseconds to sleep when process is idle
const Bmco::UInt32  IDLE_MILLISECONDS = 10;
/// max number of seconds all process needed to execute one loop
const Bmco::UInt32	MAX_SECONDS_PER_TRANS = 10;
/// 4K bytes for zero region is always enough
const Bmco::UInt32 ZERO_REGION_SIZE	= 4096;
/// interval for #0 to keep heartbeat
const Bmco::UInt64	KERNEL_HB_INTERVAL = 1000; //milliseconds
#ifdef _DEBUG
const Bmco::UInt64	KERNEL_HB_MAX_DELAY= 100000; //milliseconds
#else
const Bmco::UInt64	KERNEL_HB_MAX_DELAY= 500; //milliseconds
#endif

enum ShmAccessMode
{
	SHM_CREATE_ONLY     = 0,
	SHM_CREATE_OR_OPEN	= 1,
	SHM_OPEN_READ_WRIT	= 2,
	SHM_OPEN_READ_ONLY	= 3,
	SHM_COPY_ON_WRIT	= 4
};

template <typename SizeType>
class VirtualShm
{
public:
	VirtualShm(const char* name, ShmAccessMode option,  SizeType size=0)
	{
		switch(option){
			case SHM_CREATE_ONLY:
			case SHM_CREATE_OR_OPEN:
				//if (size == 0)???	
				_size = size;
				break;
			default:
				_size = 0;
			break;
		}
		_name = name;
		mode = option;
	}
	virtual ~VirtualShm(void){
	}

	virtual const std::string& getName(){
		return _name;
	}
	virtual ShmAccessMode getAccessMode(){
		return mode;
	}
	
	virtual bool	createOrOpen(const void *addr = 0) = 0;
	virtual bool	attach(const void *addr = 0) = 0;
	virtual void	detach() = 0;
	virtual void*	getAttachedAddress() =0;
	virtual SizeType getSize() = 0;
	virtual SizeType getFreeBytes() = 0;
	virtual bool	checkSanity() = 0;
	virtual void*	allocate(SizeType nbytes) = 0;
	virtual void*	allocate_aligned(SizeType nbytes, size_t alignment) = 0;
	virtual void	deallocate(void *addr) = 0;

protected:
	/// shm name
	std::string			_name;
	/// size in bytes
	SizeType			_size;
	ShmAccessMode		mode;
};
}
#endif



