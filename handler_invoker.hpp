
#ifndef _transparent_proxy__handler_invoker_hpp
#define _transparent_proxy__handler_invoker_hpp

#include <boost/asio.hpp>
#include <boost/cstdint.hpp>

namespace transparent_proxy {

/***************************************************************************/

template<typename Allocator, typename F>
struct preallocated_handler_invoker {
	typedef preallocated_handler_invoker<Allocator, F> this_type;

	preallocated_handler_invoker(Allocator& allocator , const F& h)
		:allocator(allocator),
		handler(h)
	{}
	preallocated_handler_invoker(const this_type& o)
		:allocator(o.allocator),
		handler(o.handler)
	{}

	friend void* asio_handler_allocate(std::size_t size, this_type* ctx) {
		return ctx->allocator.allocate(size);
	}

	friend void asio_handler_deallocate(void* ptr, std::size_t, this_type* ctx) {
		ctx->allocator.deallocate(ptr);
	}

	template <typename H>
	friend void asio_handler_invoke(const H& function, this_type* context) {
		using boost::asio::asio_handler_invoke;
		asio_handler_invoke(function, boost::addressof(context->handler));
	}

	template<typename... Args>
	void operator()(const Args&... args) {
		handler(args...);
	}

private:
	Allocator& allocator;
	F handler;
};

/***************************************************************************/

template <typename Allocator, typename F>
inline preallocated_handler_invoker<Allocator, F>
make_custom_preallocated_handler(Allocator& allocator, const F& f) {
	return preallocated_handler_invoker<Allocator, F>(allocator, f);
}

/***************************************************************************/

} // ns transparent_proxy

#endif // _transparent_proxy__handler_invoker_hpp
