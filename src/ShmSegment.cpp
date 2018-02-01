#include "ShmSegment.h"
namespace BM35 {
using Bmco::FastMutex;

ShmSegment::ShmSegment(const char* name, ShmAccessMode option, size_type size)
:_memPtr(0), VirtualShm<ShmSegment::size_type>(name, option, size)
{	
}

ShmSegment::~ShmSegment()
{
    /// we just only detach the shm object but not destroy it
    /// due to multi-process or multi-thread may share same shm segment
    detach();
}

bool ShmSegment::createOrOpen(const void *addr)
{
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (_memPtr.isNull())
    {		
        if (_size == 0)
            return false;
        try{
            switch(mode){
                case SHM_CREATE_ONLY: 
                    _memPtr = new BIP::managed_shared_memory(BIP::create_only, _name.c_str(), _size, addr);
                    break;
                case SHM_CREATE_OR_OPEN:
                    _memPtr = new BIP::managed_shared_memory(BIP::open_or_create, _name.c_str(), _size, addr);
                    break;
                default :
                    return false;
            }
        }
        catch(...){
            _memPtr = 0;
            return false;
        }
    }
    return true;
}

bool ShmSegment::attach(const void *addr)
{
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (_memPtr.isNull())
    {
        try{
            switch(mode){
                case SHM_OPEN_READ_WRIT:
                    _memPtr = new BIP::managed_shared_memory(BIP::open_only, _name.c_str(), addr);
                    break;

                case SHM_OPEN_READ_ONLY:
                    _memPtr = new BIP::managed_shared_memory(BIP::open_read_only, _name.c_str(), addr);
                    break;

                case SHM_COPY_ON_WRIT:
                    _memPtr = new BIP::managed_shared_memory(BIP::open_copy_on_write, _name.c_str(), addr);
                    break;

                default:
                    return false;
            }		
        }
        catch(...){
            return false;
        }
        _size = _memPtr->get_size();
    }
    return true;
}

void ShmSegment::detach()
{
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    _memPtr = 0;
}

bool ShmSegment::destroy()
{
    bool rtn = false;
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
    {
        switch(mode)
        {
            case SHM_CREATE_ONLY:
            case SHM_CREATE_OR_OPEN:
            {
                _memPtr = 0;
                rtn =  BIP::shared_memory_object::remove(_name.c_str());
                break;
            }
            default: /// no privilege
                break;
        }		
    }		
    return rtn;
}

bool ShmSegment::grow(size_type grow_bytes)
{
    bool rtn = false;
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
    {
        switch(mode)
        {
            case SHM_CREATE_ONLY:
            case SHM_CREATE_OR_OPEN:
            case SHM_OPEN_READ_WRIT:
                rtn = BIP::managed_shared_memory::grow(_name.c_str(), grow_bytes);
                break;
            default: /// no privilege
                break;
            }
        _size = _memPtr->get_size();
    }		
    return rtn;
}	

bool ShmSegment::destroy(const char* name)
{
    return BIP::shared_memory_object::remove(name);
}

bool ShmSegment::grow(const char* name, size_type grow_bytes)
{
    return BIP::managed_shared_memory::grow(name, grow_bytes);
}

bool ShmSegment::shrink(const char* name)
{
    return BIP::managed_shared_memory::shrink_to_fit(name);;
}
    

void* ShmSegment::getAttachedAddress()
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
        return _memPtr->get_address();
    else
        return 0;
}

ShmSegment::size_type ShmSegment::getSize()
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
        return _memPtr->get_size();
    else
        return 0;
}

ShmSegment::size_type ShmSegment::getFreeBytes()
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
        return _memPtr->get_free_memory();
    else
        return 0;
}

bool ShmSegment::checkSanity()
{
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
        return _memPtr->check_sanity();
    else
        return false;
}
    
    
void*  ShmSegment::allocate(size_type nbytes)
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
    {
        std::nothrow_t nothrow;
        switch(mode)
        {
        case SHM_CREATE_ONLY:
        case SHM_CREATE_OR_OPEN:
        case SHM_OPEN_READ_WRIT:
        case SHM_COPY_ON_WRIT:			
            return _memPtr->allocate(nbytes, nothrow);
        default:/// no privilege
            return 0;
        }
    }
    return 0;		
}
void* ShmSegment::allocate_aligned(size_type nbytes, size_t alignment)
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
    {
        std::nothrow_t nothrow;
        switch(mode)
        {
        case SHM_CREATE_ONLY:
        case SHM_CREATE_OR_OPEN:
        case SHM_OPEN_READ_WRIT:
        case SHM_COPY_ON_WRIT:			
            return _memPtr->allocate_aligned(nbytes, alignment, nothrow);
        default: /// no privilege
            return 0;
        }
    }
    return 0;
}
    
void ShmSegment::deallocate(void *addr)
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
    {		
        switch(mode)
        {
        case SHM_CREATE_ONLY:
        case SHM_CREATE_OR_OPEN:
        case SHM_OPEN_READ_WRIT:
        case SHM_COPY_ON_WRIT:			
            _memPtr->deallocate(addr);
            return;
        default: /// no privilege
            return;
        }
    }
}

} ///namespace BM35


