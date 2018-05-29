#pragma once

#include "stdafx.h"

namespace orianne {
	
	class ftp_server {
	public:
		std::string path;
		ftp_server(boost::asio::io_service& io_service, int port, std::string path);
		void start();
		
	private:
		
		boost::asio::ip::tcp::acceptor acceptor;
	};
	
}
