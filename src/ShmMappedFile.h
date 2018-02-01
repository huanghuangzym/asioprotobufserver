#ifndef __BOLSHMMAPPEDFILE_HH__
#define __BOLSHMMAPPEDFILE_HH__
#include "VirtualShm.h"

namespace BM35 {

/// need to export in DLL on Windows platform
class ShmMappedFile:public VirtualShm<BIP::managed_mapped_file::size_type>
{
public:
    typedef Bmco::SharedPtr<ShmMappedFile> Ptr;
    typedef Bmco::SharedPtr<BIP::managed_mapped_file> RawPtr;	
    typedef BIP::managed_mapped_file::handle_t	shm_internal_offset;

    typedef BIP::managed_mapped_file::size_type	size_type;
    typedef BIP::managed_mapped_file::allocator<char>::type		char_allocator;
    typedef boost::container::basic_string<char, std::char_traits<char>, char_allocator> shm_string;

public:
    /// size == 0 means only attach to an existing shm
    /// size != 0 means create or create_or_open
    /// option should match, i.e: if size is 0, option should not be SHM_CREATE_ONLY
    ShmMappedFile(const char* name, ShmAccessMode option, size_t size=0);

    ///
    /// detach a file mapped shm segment, but the shm segment and file still exists in OS enviroment
    virtual ~ShmMappedFile();

    /// create (only) shm with file backed
    /// return true when success and false if failed
    /// for portability issue, do no specify addr and let it be 0
    bool createOrOpen(const void *addr = 0);

    /// only open (read_write/read_only/copy_on_write) an existing shm with file backed
    /// return true when success and false if failed
    /// for portability issue, do no specify addr and let it be 0
    bool attach(const void *addr = 0);

    /// detach shm
    void detach();

    /// force to conduct a flush operation 
    /// you should call this function with a low frequency to avoid performance issue
    bool flush();

    /// get logical address of the attached shm segment
    void* getAttachedAddress();

    /// get current size of shm segment
    size_type getSize();

    /// get free bytes
    size_type getFreeBytes();

    /// check shm segment sanity
    bool checkSanity();

    /// raw allocate
    void* allocate(size_type nbytes);

    /// raw allocate
    void* allocate_aligned(size_type nbytes, size_t alignment);

    /// deallocate
    void deallocate(void *addr);
    
    /// get the raw ptr will enable you to access more memory management function 
    RawPtr& getRawPtr()	{
        BIP::sharable_lock<BIP::interprocess_sharable_mutex> _threadguard(threadRWMtx);
        return _memPtr;
    }

    /// Returns true if the address belongs to the managed memory segment
    bool belongsToSegment(const void* ptr)const {
        BIP::sharable_lock<BIP::interprocess_sharable_mutex> _threadguard(threadRWMtx);
        return _memPtr.isNull()? false : _memPtr->belongs_to_segment(ptr);		
    }
    /// turn a logical address to shm offset
    shm_internal_offset getOffsetFromAddr(const void *ptr) const{
        BIP::sharable_lock<BIP::interprocess_sharable_mutex> _threadguard(threadRWMtx);
        return _memPtr.isNull()? NULL :  _memPtr->get_handle_from_address(ptr);
    }
    /// turn a shm offset to a logical address
    void* getAddrFromOffset (shm_internal_offset offset) const{
        BIP::sharable_lock<BIP::interprocess_sharable_mutex> _threadguard(threadRWMtx);
        return _memPtr.isNull()? NULL : _memPtr->get_address_from_handle(offset);
    }
    
    template <class func>
    void atomic_func(func &f) {   
        BIP::sharable_lock<BIP::interprocess_sharable_mutex> _threadguard(threadRWMtx);
        if (!_memPtr.isNull()) _memPtr->atomic_func(f);
        else throw;
    }
    template <class func>
    bool try_atomic_func(func &f) {
        BIP::sharable_lock<BIP::interprocess_sharable_mutex> _threadguard(threadRWMtx);
        return _memPtr.isNull()? false : _memPtr->try_atomic_func(f); 
    }

    bool valid()const{
        BIP::sharable_lock<BIP::interprocess_sharable_mutex> _threadguard(threadRWMtx);
        return (!_memPtr.isNull());
    }

protected:
    /// *********
    /// attention: make sure no body is using the shm segment first
    /// and then you may call following functions 
    /// *********

    /// remove shm and backed file
    bool destroy();

    /// grow shm segment and backed file size (add extra grow_bytes)
    bool grow(size_type grow_bytes);
public:
    ///
    ///also provide static method to operate a named shm segment
    //
    static bool destroy(const char* name);
    static bool grow(const char* name, size_type grow_bytes);
    static bool shrink(const char* name);
private:
    ShmMappedFile();
    ShmMappedFile(const ShmMappedFile &);
    ShmMappedFile &operator=(const ShmMappedFile &);
protected:
    /// smart pointer for file mapped share memory
    RawPtr				_memPtr;
    // for thread safe
    mutable BIP::interprocess_sharable_mutex	threadRWMtx;
    // class list that can access grow/shrink/destroy function
    friend class BOLMemManager;
    friend class BOLKernel;
    friend class ControlRegionOp;
    friend class ShmMappedFileTest;
    friend class ControlRegionOpTest;
	friend class MemTaskLoop;
};
}
#endif



