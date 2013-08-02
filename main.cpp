
#include "acceptor.hpp"
#include "session.hpp"

#include <boost/thread.hpp>
#include <boost/program_options.hpp>

#include <syslog.h>
#include <string>
#include <iostream>

/***************************************************************************/

int main2(boost::asio::io_service &ios, int argc, char** argv) {
	try {
		namespace po = boost::program_options;

		std::string local_ip, remote_ip, log_file_name;
		boost::uint16_t local_port, remote_port = 0;
		std::size_t threads = 0;

		po::options_description description("allowed options");
		description.add_options()
			("help,h", "produce help message")
			("local-ip",		po::value<std::string>(&local_ip)->default_value("127.0.0.1"), "local IP-address")
			("local-port ",	po::value<boost::uint16_t>(&local_port)->default_value(44599),	"local port")
			("remote-ip",		po::value<std::string>(&remote_ip),	"remote IP-address")
			("remote-port",	po::value<boost::uint16_t>(&remote_port),	"remote port")
			("threads",			po::value<std::size_t>(&threads), "work threads")
			("log-file",      po::value<std::string>(&log_file_name)->default_value("logfile.log"), "log-file name");

		po::variables_map options;
		try {
			po::store(po::parse_command_line(argc, argv, description), options);
			po::notify(options);
		} catch ( const std::exception& ex ) {
			std::cout << "command line error: " << ex.what() << std::endl;
			return 1;
		}
		if ( options.count("help") ) {
			std::cout << description << std::endl;
			return 0;
		}
		if ( !options.count("remote-ip") ) {
			std::cout << "please choose remote IP" << std::endl;
			return 0;
		}
		if ( !options.count("remote-port") ) {
			std::cout << "please choose remote port" << std::endl;
			return 0;
		}

		transparent_proxy::acceptor acceptor(ios, local_ip, local_port, remote_ip, remote_port);
		acceptor.start_accept();

		if ( !options.count("threads") ) {
			ios.run();
		} else {
			if ( !threads ) {
				std::cout << "please choose work threads count" << std::endl;
				return 0;
			}

			boost::thread_group tg;
			for ( std::size_t idx = 0; idx < threads; ++idx ) {
				tg.create_thread(boost::bind(&boost::asio::io_service::run, &ios));
			}

			tg.join_all();
		}
	} catch ( const std::exception& ex ) {
		std::cout << "(exception): " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

/***************************************************************************/

int main(int argc, char **argv) {
	try {
		boost::asio::io_service ios;

		boost::asio::signal_set signals(ios, SIGINT, SIGTERM);
		signals.async_wait(boost::bind(&boost::asio::io_service::stop, &ios));
		ios.notify_fork(boost::asio::io_service::fork_prepare);

		if (pid_t pid = fork()) {
			if (pid > 0) {
				exit(0);
			} else {
				syslog(LOG_ERR | LOG_USER, "First fork failed: %m");
				return 1;
			}
		}

		setsid();
		chdir("/");
		umask(0);

		if (pid_t pid = fork()) {
			if (pid > 0) {
				exit(0);
			} else {
				syslog(LOG_ERR | LOG_USER, "Second fork failed: %m");
				return 1;
			}
		}

		close(0);
		close(1);
		close(2);

		if (open("/dev/null", O_RDONLY) < 0) {
			syslog(LOG_ERR | LOG_USER, "Unable to open /dev/null: %m");
			return 1;
		}

		const char* output = "/tmp/tcp-proxy.log";
		const int flags = O_WRONLY | O_CREAT | O_APPEND;
		const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
		if (open(output, flags, mode) < 0) {
			syslog(LOG_ERR | LOG_USER, "Unable to open output file %s: %m", output);
			return 1;
		}

		if (dup(1) < 0) {
			syslog(LOG_ERR | LOG_USER, "Unable to dup output descriptor: %m");
			return 1;
		}

		ios.notify_fork(boost::asio::io_service::fork_child);

		syslog(LOG_INFO | LOG_USER, "Daemon started");
		int res = main2(ios, argc, argv);
		syslog(LOG_INFO | LOG_USER, "Daemon stopped with code=%d", res);
	} catch (std::exception& e) {
		syslog(LOG_ERR | LOG_USER, "Exception: %s", e.what());
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
}

/***************************************************************************/
