#include "ftp/stdafx.h"

#include "ftp/ftp_server.h"
#include "ftp/ftp_console.h"
#include "ftp/ftp_session.h"

using boost::asio::ip::tcp;
using namespace std;

orianne::ftp_server::ftp_server(boost::asio::io_service& io_service, int port, std::string path)
: acceptor(io_service, tcp::endpoint(tcp::v4(), port)) {
	this->path = path;
	start();
}

struct connection_handler : boost::enable_shared_from_this<connection_handler> {
	
	explicit connection_handler(boost::asio::io_service& service, std::string path)
	: socket(service), session(service, socket), console(session)
	{
		session.set_root_directory(boost::filesystem::path{path});
	}
	
	typedef boost::shared_ptr<connection_handler> ptr;
	
	static ptr create(boost::asio::io_service& service, std::string path) {
		return ptr(new connection_handler(service, path));
	}
	
	tcp::socket socket;
	orianne::ftp_session session;
	orianne::ftp_console console;
	boost::asio::streambuf buf;
	
	void handle_connect(const boost::system::error_code code, orianne::ftp_server* server) {
		cout << "handle_connection()" << endl;
		
		console.set_write_callback(boost::bind(&connection_handler::write_message,
											   this, _1));
		
		boost::asio::async_write(socket,
								 boost::asio::buffer(console.greeter()),
								 boost::bind(&connection_handler::handle_write, shared_from_this(),
											 boost::asio::placeholders::error,
											 boost::asio::placeholders::bytes_transferred));
		
		trigger_read();
		
		server->start();
	}
	
	void trigger_read() {
		if(socket.is_open()){
			boost::asio::async_read_until(socket, buf, "\n",
										  boost::bind(&connection_handler::handle_read, shared_from_this()));
		}
	}
	
	void handle_write(const boost::system::error_code& /*error*/, size_t
	/*bytes_transferred*/) {
		
	}
	
	void handle_read() {
		std::istream is(&buf);
		std::string s;
		getline(is, s);
		if(s.size() > 0) {
			console.read_line(s);
			
			trigger_read();
		}
	}
	
	void write_message(const std::string& message) {
		const char* buf = message.c_str();
		std::string *str = new std::string(buf);
		str->append("\r\n");
		cout << "Message: " << *str << endl;
		boost::asio::async_write(socket, boost::asio::buffer(*str),
								 boost::bind(&connection_handler::dispose_write_buffer,
											 shared_from_this(), boost::asio::placeholders::error,
											 boost::asio::placeholders::bytes_transferred));
	}
	
	void dispose_write_buffer(const boost::system::error_code& /*error*/,
							  size_t /*bytes_transferred*/) {
		
	}
};

void orianne::ftp_server::start() {
	cout << "start()" << endl;
	
	connection_handler::ptr handler =
	connection_handler::create(acceptor.get_io_service(), path);
	boost::shared_ptr<connection_handler>& sptr(handler);
	
	acceptor.async_accept(handler->socket,
						  boost::bind(&connection_handler::handle_connect, sptr,
									  boost::asio::placeholders::error, this));
}
