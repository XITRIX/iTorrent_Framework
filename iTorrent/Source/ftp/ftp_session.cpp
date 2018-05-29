#include "ftp/stdafx.h"

#include "ftp/ftp_session.h"

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

using boost::asio::ip::tcp;

orianne::ftp_session::ftp_session(boost::asio::io_service& _service, boost::asio::ip::tcp::socket& socket_)
: io_service(_service), acceptor(0), working_directory("/"), socket(socket_)
{
}

void orianne::ftp_session::set_root_directory(boost::filesystem::path const& directory) {
	root_directory = directory;
}

orianne::ftp_result orianne::ftp_session::set_username(const std::string& username) {
	return orianne::ftp_result(331, "Please enter your password.");
}

orianne::ftp_result orianne::ftp_session::set_password(const std::string& username) {
	return orianne::ftp_result(230, "Login successful.");
}

static std::string endpoint_to_string(boost::asio::ip::address_v4::bytes_type address, unsigned short port) {
	std::stringstream stream;
	stream << "(";
	for(int i=0; i<4; i++)
		stream << (int)address[i] << ",";
	stream << ((port >> 8) & 0xff) << "," << (port & 0xff) << ")";
	
	return stream.str();
}

orianne::ftp_result orianne::ftp_session::set_passive() {
	static unsigned short port = 10000;
	
	if(acceptor == 0)
		acceptor = new tcp::acceptor(io_service, tcp::endpoint(tcp::v4(), ++port));
	
	std::string tmp_message = "Entering passive mode ";
	tmp_message.append(endpoint_to_string(socket.local_endpoint().address().to_v4().to_bytes(), port));
	
	return orianne::ftp_result (227, tmp_message);
}

orianne::ftp_result orianne::ftp_session::get_size(const std::string& filename) {
	std::stringstream stream;
	stream << boost::filesystem::file_size(root_directory / working_directory / filename);
	
	return orianne::ftp_result(213, stream.str());
}

orianne::ftp_result orianne::ftp_session::change_working_directory(const std::string& new_directory) {
	if (new_directory[0] != '/') {
		if (new_directory.compare("..") == 0) {
			working_directory = working_directory.parent_path();
		} else {
			working_directory = working_directory / boost::filesystem::path(new_directory);
		}
	} else {
		std::string s = "/..";
		if (std::equal(s.rbegin(), s.rend(), new_directory.rbegin())) {
			working_directory = working_directory.parent_path();
		} else {
			working_directory.assign(new_directory, boost::filesystem::path::codecvt());
		}
	}
	return orianne::ftp_result(250, "OK");
}

orianne::ftp_result orianne::ftp_session::create_new_directory(const std::string& new_directory) {
	if (new_directory[0] != '/') {
		boost::filesystem::path path = root_directory / working_directory / boost::filesystem::path(new_directory);
		if (mkdir(path.string().c_str(), ACCESSPERMS) == 0) {
			return orianne::ftp_result(257, "\"" + path.string() + "\"");
		} else {
			return orianne::ftp_result(550, "Error creating directory.");
		}
	} else {
		boost::filesystem::path path(new_directory);
		path = root_directory / path;
		std::cout << path.string() << 1 <<std::endl;
		if (mkdir(path.string().c_str(), ACCESSPERMS) == 0) {
			return orianne::ftp_result(257, "\"" + path.string() + "\"");
		} else {
			return orianne::ftp_result(550, "Error creating directory.");
		}
	}
}

orianne::ftp_result orianne::ftp_session::remove_directory(const std::string& new_directory) {
	if (new_directory[0] != '/') {
		boost::filesystem::path path = root_directory / working_directory / boost::filesystem::path(new_directory);
		if (remove(path.string().c_str()) == 0) {
			return orianne::ftp_result(250, "OK");
		} else {
			return orianne::ftp_result(550, "Error removing directory.");
		}
	} else {
		boost::filesystem::path path(new_directory);
		path = root_directory / path;
		if (remove(path.string().c_str()) == 0) {
			return orianne::ftp_result(250, "OK");
		} else {
			return orianne::ftp_result(550, "Error removing directory.");
		}
	}
}

orianne::ftp_result orianne::ftp_session::rename_file_from(const std::string& new_directory) {
	if (new_directory[0] != '/') {
		boost::filesystem::path path = root_directory / working_directory / boost::filesystem::path(new_directory);
		rename_from_path = path.string();
	} else {
		boost::filesystem::path path(new_directory);
		path = root_directory / path;
		rename_from_path = path.string();
	}
	return orianne::ftp_result(350, "Waiting for target path.");
}

orianne::ftp_result orianne::ftp_session::rename_file_to(const std::string& new_directory) {
	if (rename_from_path.empty()) {
		return orianne::ftp_result(503, "Use RNFR before RNTO.");
	}
	if (new_directory[0] != '/') {
		boost::filesystem::path path = root_directory / working_directory / boost::filesystem::path(new_directory);
		if (rename(rename_from_path.c_str(), path.string().c_str()) == 0) {
			return orianne::ftp_result(250, "OK");
		} else {
			return orianne::ftp_result(550, "Error renaming file.");
		}
	} else {
		boost::filesystem::path path(new_directory);
		path = root_directory / path;
		if (rename(rename_from_path.c_str(), path.string().c_str()) == 0) {
			return orianne::ftp_result(250, "OK");
		} else {
			return orianne::ftp_result(550, "Error renaming file.");
		}
	}
}

orianne::ftp_result orianne::ftp_session::set_type(const struct orianne::ftp_transfer_type& type) {
	return orianne::ftp_result(200, "Switching to Binary mode.");
}

orianne::ftp_result orianne::ftp_session::get_working_directory() {
	return orianne::ftp_result(257, "\"" + working_directory.string() + "\"");
}

orianne::ftp_result orianne::ftp_session::get_system() {
	return orianne::ftp_result(215, "iOS");
}

static std::string get_list(const boost::filesystem::path& path) {
	using namespace boost::filesystem;
	std::stringstream stream;
	
	for(directory_iterator it(path); it != directory_iterator(); it++) {
		struct stat t_stat;
		stat(it->path().string().c_str(), &t_stat);
		struct tm* timeinfo = localtime(&t_stat.st_ctime);
		char date[80];
		strftime(date, sizeof(date), "%b %e %Y", timeinfo);
		
		bool dir = is_directory(it->path());
		stream << boost::format("%crw-rw-rw-   1 %-10s %-10s %10lu %s %s\r\n")
		% (dir ? 'd' : '-')
		% "iTorrent" % "iTorrent"
		% (dir ? 0 : file_size(it->path()))
		% date
		% it->path().filename().string();
	}
	std::cout << "STR: \n" << stream.str() << std::endl;
	return stream.str();
}

template<typename T> struct dumper : boost::enable_shared_from_this<T> {
	boost::asio::io_service& service;
	tcp::socket socket;
	boost::function<void (const orianne::ftp_result&)> callback;
	
	explicit dumper(boost::function<void (const orianne::ftp_result&)> cb, boost::asio::io_service& service_)
	: service(service_), socket(service), callback(cb)
	{
	}
	
	static boost::shared_ptr<T> create(boost::function<void (const orianne::ftp_result&)> cb, boost::asio::io_service& service) {
		return boost::shared_ptr<T>(new T(cb, service));
	}
	
	void async_wait(boost::asio::ip::tcp::acceptor& acceptor) {
		acceptor.async_accept(socket,
							  boost::bind(&T::handle_connect, this->shared_from_this()));
	}
};

struct dir_list_dumper : dumper<dir_list_dumper> {
	std::string data;
	
	explicit dir_list_dumper(boost::function<void (const orianne::ftp_result&)> cb, boost::asio::io_service& service)
	: dumper(cb, service)
	{
	}
	
	void handle_connect() {
		boost::asio::async_write(socket,
								 boost::asio::buffer(data),
								 boost::bind(&dir_list_dumper::handle_write, shared_from_this()));
		callback(orianne::ftp_result(150, "Sending directory listing."));
	}
	
	void handle_write() {
		callback(orianne::ftp_result(226, "Done."));
	}
	
	void set_data(const std::string& data_) {
		data = data_;
	}
};

void orianne::ftp_session::list(boost::function<void (const orianne::ftp_result&)> cb) {
	boost::shared_ptr<dir_list_dumper> dumper = dir_list_dumper::create(cb, io_service);
	dumper->set_data(get_list(root_directory / working_directory));
	dumper->async_wait(*acceptor);
}

struct file_dumper : dumper<file_dumper> {
	std::ifstream stream;
	char buffer[1024];
	boost::asio::mutable_buffers_1 m_buffer;
	
	explicit file_dumper(boost::function<void (const orianne::ftp_result&)> cb, boost::asio::io_service& service, const std::string& path)
	: dumper(cb, service), stream(path.c_str(), std::ios::in | std::ios::binary), m_buffer(buffer, 1024)
	{
	}
	
	static boost::shared_ptr<file_dumper> create(boost::function<void (const orianne::ftp_result&)> cb, boost::asio::io_service& service, const std::string& path) {
		return boost::shared_ptr<file_dumper>(new file_dumper(cb, service, path));
	}
	
	void handle_connect() {
		callback(orianne::ftp_result(150, "Sending file contents."));
		
		handle_write();
	}
	
	void handle_write() {
		stream.read(buffer, 1024);
		std::streamsize count = stream.gcount();
		
		if(count == 0) {
			callback(orianne::ftp_result(226, "Done."));
		} else {
			if(count < 1024)
				m_buffer = boost::asio::buffer(buffer, count);
			
			boost::asio::async_write(socket,
									 m_buffer,
									 boost::bind(&file_dumper::handle_write, shared_from_this()));
		}
	}
	
	~file_dumper() {
	}
};

struct file_loader : dumper<file_loader> {
	std::ofstream stream;
	char buffer[4096];
	
	explicit file_loader(boost::function<void (const orianne::ftp_result&)> cb, boost::asio::io_service& service, const std::string& path)
	: dumper(cb, service), stream(path.c_str(), std::ios::out | std::ios::binary)
	{
	}
	
	static boost::shared_ptr<file_loader> create(boost::function<void (const orianne::ftp_result&)> cb, boost::asio::io_service& service, const std::string& path) {
		return boost::shared_ptr<file_loader>(new file_loader(cb, service, path));
	}
	
	void handle_connect() {
		callback(orianne::ftp_result(150, "Receaving file contents."));
		
		boost::asio::async_read(socket,
								boost::asio::buffer(buffer, 4096),
								boost::asio::transfer_at_least(1),
								boost::bind(&file_loader::handle_read,
											shared_from_this(),
											boost::asio::placeholders::bytes_transferred));
	}
	
	void handle_read(std::size_t recvlen) {
		size_t count = recvlen;
		std::cout << "buffer size: " << count << std::endl;
		
		if(count == 0) {
			callback(orianne::ftp_result(226, "Done."));
		} else {
			stream.write(buffer, count);
			strcpy(buffer, "");
			boost::asio::async_read(socket,
									boost::asio::buffer(buffer, 4096),
									boost::asio::transfer_all(),
									boost::bind(&file_loader::handle_read,
												shared_from_this(),
												boost::asio::placeholders::bytes_transferred));
		}
	}
	
	~file_loader() {
	}
};

void orianne::ftp_session::store(const std::string& filename, boost::function<void (const orianne::ftp_result&)> cb) {
	boost::filesystem::path path = root_directory / working_directory / filename;
	
	std::cout << "Opening " << path.make_preferred() << " for upload" << std::endl;
	
	boost::shared_ptr<file_loader> dumper = file_loader::create(cb, io_service, path.make_preferred().string());
	dumper->async_wait(*acceptor);
}

void orianne::ftp_session::retrieve(const std::string& filename, boost::function<void (const orianne::ftp_result&)> cb) {
	boost::filesystem::path path = root_directory / working_directory / filename;
	
	std::cout << "Opening " << path.make_preferred() << " for download" << std::endl;
	
	boost::shared_ptr<file_dumper> dumper = file_dumper::create(cb, io_service, path.make_preferred().string());
	dumper->async_wait(*acceptor);
}

orianne::ftp_transfer_type orianne::read_transfer_type(std::istream& stream) {
	orianne::ftp_transfer_type transfer_type;
	transfer_type.type = 'I';
	return transfer_type;
}
