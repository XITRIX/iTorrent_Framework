#pragma once

#include "stdafx.h"

#include <boost/filesystem.hpp>

namespace orianne {
	
	struct ftp_transfer_type {
		unsigned char type;
		unsigned char format;
		unsigned byte_size;
	};
	
	ftp_transfer_type read_transfer_type(std::istream& stream);
	
	struct ftp_result {
		ftp_result() {
		}
		
		ftp_result(unsigned code_, const std::string& message_)
		: code(code_), message(message_) {
		}
		
		unsigned code;
		std::string message;
	};
	
	class ftp_session {
		boost::asio::io_service& io_service;
		boost::asio::ip::tcp::acceptor* acceptor;
		boost::asio::ip::tcp::socket& socket;
		
		boost::filesystem::path root_directory;
		boost::filesystem::path working_directory;
		
	public:
		explicit ftp_session(boost::asio::io_service&, boost::asio::ip::tcp::socket& socket);
		
		void set_root_directory(const boost::filesystem::path& root_directory);
		std::string rename_from_path;
		
		ftp_result set_username(const std::string& username);
		ftp_result set_password(const std::string& password);
		ftp_result quit();
		ftp_result set_port(unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
		ftp_result set_type(const ftp_transfer_type& type);
		ftp_result get_system();
		ftp_result set_mode(unsigned char mode);
		ftp_result set_file_structure(unsigned char stru);
		ftp_result get_working_directory();
		ftp_result create_new_directory(const std::string& directory);
		ftp_result change_working_directory(const std::string& directory);
		ftp_result remove_directory(const std::string& directory);
		ftp_result remove_file(const std::string& filemane);
		ftp_result rename_file_from(const std::string& filemane);
		ftp_result rename_file_to(const std::string& filemane);
		ftp_result get_size(const std::string& filename);
		ftp_result set_passive();
		ftp_result store(const std::string& filename);
		ftp_result no_operation();
		
		void retrieve(const std::string& filename, boost::function<void (const ftp_result&)> cb);
		void store(const std::string& filename, boost::function<void (const ftp_result&)> cb);
		void list(boost::function<void (const ftp_result&)> cb);
	};
	
}
