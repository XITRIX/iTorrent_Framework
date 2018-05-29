#include "ftp/stdafx.h"

#include "ftp/ftp_console.h"

#include <boost/lambda/lambda.hpp>

orianne::ftp_console::ftp_console(ftp_session& _session)
: session(_session), greeter_("220 orianne Ready.\r\n")
{
}

static void build_result_mesg(std::string& mesg, orianne::ftp_result result) {
	std::stringstream stream;
	stream << result.code << " " << result.message;
	mesg = stream.str();
}

static void write_result(const orianne::ftp_result& result, orianne::ftp_console::write_message_func write_message) {
	std::string out_mesg;
	build_result_mesg(out_mesg, result);
	std::cout << " > " << out_mesg << std::endl;
	write_message(out_mesg);
}

void orianne::ftp_console::read_line(const std::string& mesg) {
	std::cout << " < " << mesg << std::endl;
	
	std::stringstream stream(mesg);
	
	std::string command;
	stream >> command;
	
	ftp_result result;
	
	if(command == "USER") {
		std::string username = stream.str().erase(0, command.length() + 1);
		username.pop_back();
		result = session.set_username(username);
	} else if (command == "PASS") {
		std::string password = stream.str().erase(0, command.length() + 1);
		password.pop_back();
		result = session.set_password(password);
	} else if (command == "SYST") {
		result = session.get_system();
	} else if (command == "PWD") {
		result = session.get_working_directory();
	} else if (command == "TYPE") {
		orianne::ftp_transfer_type type = read_transfer_type(stream);
		result = session.set_type(type);
	} else if (command == "PASV") {
		result = session.set_passive();
	} else if (command == "SIZE") {
		std::string filename = stream.str().erase(0, command.length() + 1);
		filename.pop_back();
		result = session.get_size(filename);
	} else if (command == "LIST") {
		session.list(boost::bind(&write_result, _1, write_message));
		return;
	} else if (command == "RETR") {
		session.retrieve(mesg.substr(5, mesg.length() - 6), boost::bind(&write_result, _1, write_message));
		return;
	} else if (command == "STOR") {
		session.store(mesg.substr(5, mesg.length() - 6), boost::bind(&write_result, _1, write_message));
		return;
	} else if (command == "CWD") {
		std::string directory = stream.str().erase(0, command.length() + 1);
		directory.pop_back();
		result = session.change_working_directory(directory);
	} else if (command == "MKD") {
		std::string directory = stream.str().erase(0, command.length() + 1);
		directory.pop_back();
		result = session.create_new_directory(directory);
	} else if (command == "RMD" || command == "DELE") {
		std::string directory = stream.str().erase(0, command.length() + 1);
		directory.pop_back();
		result = session.remove_directory(directory);
	} else if (command == "RNFR") {
		std::string directory = stream.str().erase(0, command.length() + 1);
		directory.pop_back();
		result = session.rename_file_from(directory);
	} else if (command == "RNTO") {
		std::string directory = stream.str().erase(0, command.length() + 1);
		directory.pop_back();
		result = session.rename_file_to(directory);
	} else {
		result.code = 500;
		result.message = "Syntax Error.";
	}
	
	write_result(result, write_message);
}

void orianne::ftp_console::set_write_callback(boost::function<void (const std::string&)> wm) {
	write_message = wm;
}

const std::string& orianne::ftp_console::greeter() { return greeter_; }
