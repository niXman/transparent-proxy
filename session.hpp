
#ifndef _transparent_proxy__session_hpp
#define _transparent_proxy__session_hpp

#include "handler_allocator.hpp"
#include "handler_invoker.hpp"

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>

#include <iostream>

namespace transparent_proxy {

/***************************************************************************/

struct session: boost::enable_shared_from_this<session> {
	enum { default_buffer_size = 1024*8 };

	session(boost::asio::io_service& io_service)
		:_client_socket(io_service),
		_proxy_socket(io_service)
	{}

	boost::asio::ip::tcp::socket& get_socket() {
		return _client_socket;
	}

	void start(const std::string& proxy_ip, std::size_t proxy_port) {
		_proxy_socket.async_connect(
			boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(proxy_ip), proxy_port),
			boost::bind(
				&session::handle_connect,
				shared_from_this(),
				boost::asio::placeholders::error
			)
		);
	}

	void handle_connect(const boost::system::error_code& error) {
		if ( !error ) {
			_proxy_socket.async_read_some(
				boost::asio::buffer(_proxy_buffer, default_buffer_size),
				make_custom_preallocated_handler(
					_proxy_socket_allocator,
					boost::bind(
						&session::handle_proxy_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
					)
				)
			);

			_client_socket.async_read_some(
				boost::asio::buffer(_client_buffer, default_buffer_size),
				make_custom_preallocated_handler(
					_client_socket_allocator,
					boost::bind(
						&session::handle_client_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
					)
				)
			);
		} else {
			std::cerr << __FUNCTION__ << ": " << error.message() << std::endl;
			close();
		}
	}

	void handle_client_read(const boost::system::error_code& error, const std::size_t bytes_transferred) {
		if ( !error ) {
			boost::asio::async_write(
				_proxy_socket,
				boost::asio::buffer(_client_buffer, bytes_transferred),
				make_custom_preallocated_handler(
					_proxy_socket_allocator,
					boost::bind(
						&session::handle_proxy_write,
						shared_from_this(),
						boost::asio::placeholders::error
					)
				)
			);
		} else {
			std::cerr << __FUNCTION__ << ": " << error.message() << std::endl;
			close();
		}
	}

	void handle_proxy_write(const boost::system::error_code& error) {
		if ( !error ) {
			_client_socket.async_read_some(
				boost::asio::buffer(_client_buffer, default_buffer_size),
				make_custom_preallocated_handler(
					_client_socket_allocator,
					boost::bind(
						&session::handle_client_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
					)
				)
			);
		} else {
			std::cerr << __FUNCTION__ << ": " << error.message() << std::endl;
			close();
		}
	}

	void handle_proxy_read(const boost::system::error_code& error, const std::size_t bytes_transferred) {
		if ( !error ) {
			boost::asio::async_write(
				_client_socket,
				boost::asio::buffer(_proxy_buffer, bytes_transferred),
				make_custom_preallocated_handler(
					_client_socket_allocator,
					boost::bind(
						&session::handle_client_write,
						shared_from_this(),
						boost::asio::placeholders::error
					)
				)
			);
		} else if ( error == boost::asio::error::eof ) {
			close();
		} else {
			std::cerr << __FUNCTION__ << ": " << error.message() << std::endl;
			close();
		}
	}

	void handle_client_write(const boost::system::error_code& error) {
		if ( !error ) {
			_proxy_socket.async_read_some(
				boost::asio::buffer(_proxy_buffer, default_buffer_size),
				make_custom_preallocated_handler(
					_proxy_socket_allocator,
					boost::bind(
						&session::handle_proxy_read,
						shared_from_this(),
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred
					)
				)
			);
		} else {
			std::cerr << __FUNCTION__ << ": " << error.message() << std::endl;
			close();
		}
	}

	void close() {
		if ( _client_socket.is_open() ) {
			_client_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			_client_socket.close();
		}
		if ( _proxy_socket.is_open() ) {
			_proxy_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
			_proxy_socket.close();
		}
	}

private:
	enum { for_handler_allocator_size = 512 };

	boost::asio::ip::tcp::socket _client_socket;
	in_stack_handler_allocator<for_handler_allocator_size> _client_socket_allocator;
	boost::array<char, default_buffer_size> _client_buffer;

	boost::asio::ip::tcp::socket _proxy_socket;
	in_stack_handler_allocator<for_handler_allocator_size> _proxy_socket_allocator;
	boost::array<char, default_buffer_size> _proxy_buffer;
};

/***************************************************************************/

} //  ns transparent_proxy

#endif // _transparent_proxy__session_hpp
