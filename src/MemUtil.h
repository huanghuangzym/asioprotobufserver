#ifndef __MEMUTIL_HH__
#define __MEMUTIL_HH__
#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/managed_heap_memory.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/permissions.hpp>
#include <boost/interprocess/segment_manager.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/allocators/allocator.hpp>


#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_condition_any.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_recursive_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/interprocess/sync/lock_options.hpp>
#include <boost/interprocess/sync/named_condition.hpp> 
#include <boost/interprocess/sync/named_condition_any.hpp> 
#include <boost/interprocess/sync/named_mutex.hpp> 
#include <boost/interprocess/sync/named_recursive_mutex.hpp> 
#include <boost/interprocess/sync/named_semaphore.hpp> 
#include <boost/interprocess/sync/named_sharable_mutex.hpp>
#include <boost/interprocess/sync/named_upgradable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp> 
#include <boost/interprocess/sync/sharable_lock.hpp> 
#include <boost/interprocess/sync/upgradable_lock.hpp> 
#include <boost/interprocess/detail/move.hpp>

#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>


#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>

//add by xuyun
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/allocators/node_allocator.hpp>
#include <boost/thread.hpp>


namespace BIP = boost::interprocess;
namespace BMI = boost::multi_index;

#endif



