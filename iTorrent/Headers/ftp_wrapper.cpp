//
//  ftp_wrapper.cpp
//  iTorrent
//
//  Created by Daniil Vinogradov on 23.05.2018.
//  Copyright © 2018  XITRIX. All rights reserved.
//

#include "ftp/ftp_server.h"
#include "ftp/stdafx.h"
#include <stdio.h>

using namespace std;

class FTP {
private:
	boost::asio::io_service io_service;
public:
	static FTP* standart;
	FTP(int port, string path) {
		try {
			standart = this;
			orianne::ftp_server server(io_service, port, path);
			io_service.run();
		} catch(std::exception& e) {
			std::cout << e.what() << std::endl;
		}
	}
	
	void stop() {
		io_service.stop();
		standart = NULL;
	}
};

FTP* FTP::standart;
extern "C" void ftp_start(int port, char* path) {
	new FTP(port, string(path));
}

extern "C" void ftp_stop() {
	FTP::standart->stop();
}
