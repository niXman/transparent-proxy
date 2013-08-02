
#ifndef _transparent_proxy__handler_allocator_hpp
#define _transparent_proxy__handler_allocator_hpp

#include <boost/noncopyable.hpp>
#include <boost/aligned_storage.hpp>
#include <boost/cstdint.hpp>

namespace transparent_proxy {

/***************************************************************************/

template <size_t alloc_size>
struct in_stack_handler_allocator :private boost::noncopyable {
  in_stack_handler_allocator()
	 :_in_use(false)
  {}

  ~in_stack_handler_allocator() {}

  void* allocate(size_t size) {
	  if ( !_in_use && (size <= _storage.size) ) {
		  _in_use = true;
		  return _storage.address();
	  }
	  return ::operator new(size);
  }

  void deallocate(void* pointer) {
	  if ( _storage.address() == pointer ) {
		  _in_use = false;
		  return;
	  }
	  ::operator delete(pointer);
  }

private:
  boost::aligned_storage<alloc_size> _storage;
  bool _in_use;
};

/***************************************************************************/

} // ns transparent_proxy

#endif // _transparent_proxy__handler_allocator_hpp
