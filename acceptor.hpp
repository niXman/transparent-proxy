
#ifndef _transparent_proxy__acceptor_hpp
#define _transparent_proxy__acceptor_hpp

#include "session.hpp"

#include <boost/bind.hpp>
#include <boost/asio.hpp>

namespace transparent_proxy {

/***************************************************************************/

struct acceptor {
	acceptor(
		boost::asio::io_service& io_service,
		const std::string& local_ip,
		boost::uint16_t local_port,
		const std::string remote_ip,
		boost::uint16_t remote_port
	):	_io_service(io_service),
		_acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(local_ip), local_port)),
		_remote_ip(remote_ip),
		_remote_port(remote_port)
	{}

	void start_accept() {
		try {
			boost::shared_ptr<session> _connection(new session(_io_service));
			_acceptor.async_accept(
				_connection->get_socket(),
				boost::bind(
					&acceptor::handle_accept,
					this,
					_connection,
					boost::asio::placeholders::error
				)
			);
		} catch ( const std::exception& ex ) {
			std::cerr << __FUNCTION__ << ": " << "acceptor exception: " << ex.what() << std::endl;
		}
	}

	void handle_accept(boost::shared_ptr<session> connection, const boost::system::error_code& error) {
		if ( !error ) {
			connection->start(_remote_ip, _remote_port);
			start_accept();
		} else {
			std::cerr << __FUNCTION__ << ": " << error.message() << std::endl;
		}
	}

private:
	boost::asio::io_service& _io_service;
	boost::asio::ip::tcp::acceptor _acceptor;
	std::string _remote_ip;
	boost::uint16_t _remote_port;
};

/***************************************************************************/

} // ns transparent_proxy

#endif // _transparent_proxy__acceptor_hpp
