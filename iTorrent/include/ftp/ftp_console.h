#pragma once

#include "stdafx.h"

#include "ftp_session.h"

#include <map>

namespace orianne {
	
	class ftp_console {
	public:
		typedef boost::function<void (const std::string&)> write_message_func;
		typedef boost::function<void (const std::string&, ftp_session&, write_message_func&)> command_func;
		
		ftp_console(ftp_session& _session);
		void read_line(const std::string& mesg);
		void set_write_callback(write_message_func wm);
		const std::string& greeter();
		
	private:
		ftp_session& session;
		write_message_func write_message;
		std::string greeter_;
		
		static std::map<std::string, command_func>& commands();
	};
	
}
