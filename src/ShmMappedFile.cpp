#include "ShmMappedFile.h"

namespace BM35 {
using Bmco::FastMutex;
ShmMappedFile::ShmMappedFile(const char* name, ShmAccessMode option, size_type size)
:_memPtr(0), VirtualShm<ShmMappedFile::size_type>(name, option, size)
{
}

ShmMappedFile::~ShmMappedFile()
{
    /// we just only detach the shm object but not destroy it
    /// due to multi-process or multi-thread may share same shm segment
    detach();
}

bool ShmMappedFile::createOrOpen(const void *addr)
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
                    _memPtr = new BIP::managed_mapped_file(BIP::create_only, _name.c_str(), _size, addr);
                break;
                case SHM_CREATE_OR_OPEN:
                    _memPtr = new BIP::managed_mapped_file(BIP::open_or_create, _name.c_str(), _size, addr);
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

bool ShmMappedFile::attach(const void *addr)
{
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (_memPtr.isNull())
    {
        try{
            switch(mode){
                case SHM_OPEN_READ_WRIT:
                    _memPtr = new BIP::managed_mapped_file(BIP::open_only, _name.c_str(), addr);
                    break;

                case SHM_OPEN_READ_ONLY:
                    _memPtr = new BIP::managed_mapped_file(BIP::open_read_only, _name.c_str(), addr);
                    break;

                case SHM_COPY_ON_WRIT:
                    _memPtr = new BIP::managed_mapped_file(BIP::open_copy_on_write, _name.c_str(), addr);
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

void ShmMappedFile::detach()
{
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    _memPtr = 0;
}

bool ShmMappedFile::destroy()
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
                rtn = BIP::file_mapping::remove(_name.c_str());		
                break;
            }
            default: /// no privilege
                break;
        }		
    }		
    return rtn;
}

bool ShmMappedFile::grow(size_type grow_bytes)
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
                rtn = BIP::managed_mapped_file::grow(_name.c_str(), grow_bytes);
                break;
            default: /// no privilege
                break;
            }
        _size = _memPtr->get_size();
    }		
    return rtn;
}
    
bool ShmMappedFile::flush()
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
            rtn = _memPtr->flush();
            break;
        default: /// no privilege
            break;
        }
    }
    return rtn;
}

bool ShmMappedFile::destroy(const char* name)
{
    return BIP::file_mapping::remove(name);
}

bool ShmMappedFile::grow(const char* name, size_type grow_bytes)
{
    return BIP::managed_mapped_file::grow(name, grow_bytes);
}

bool ShmMappedFile::shrink(const char* name)
{
    return BIP::managed_mapped_file::shrink_to_fit(name);;
}
    

void* ShmMappedFile::getAttachedAddress()
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
        return _memPtr->get_address();
    else
        return 0;
}

ShmMappedFile::size_type ShmMappedFile::getSize()
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
        return _memPtr->get_size();
    else
        return 0;
}

ShmMappedFile::size_type ShmMappedFile::getFreeBytes()
{
    BIP::sharable_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
        return _memPtr->get_free_memory();
    else
        return 0;
}

bool ShmMappedFile::checkSanity()
{
    BIP::scoped_lock<BIP::interprocess_sharable_mutex>
        _threadguard(threadRWMtx);
    if (!_memPtr.isNull())
        return _memPtr->check_sanity();
    else
        return false;
}
    
    
void*  ShmMappedFile::allocate(size_type nbytes)
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
void* ShmMappedFile::allocate_aligned(size_type nbytes, size_t alignment)
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
    
void ShmMappedFile::deallocate(void *addr)
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


