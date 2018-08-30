#include <iostream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

int g_port = 45455;
int g_port1 = 1241; //If you want to open 2 apps on 1 pc

class Messenger
{
public:

	//Construtcor
	Messenger(boost::asio::io_service& ios):
	  sock(ios),acceptor(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(),g_port))
	{ listen();}
	
	void send_message(std::string msg)
	{ sock.write_some(boost::asio::buffer(msg + '\n'));	}
	
	void connect_to(std::string ip)
	{
		boost::system::error_code err;
		boost::asio::ip::address_v4 reciever_addr = boost::asio::ip::address_v4::from_string(ip,err);
		if(err)
		{
			std::cerr<<"Invalid ip. Please try again.\n";
			return;
		}
		if(!get_connected())
		{
			sock.connect(boost::asio::ip::tcp::endpoint(reciever_addr,g_port1),err);
			if(err)
			{
				listen();
				std::cerr<<"Connection failure.\nerr.message="<<err.message()<<'\n';
				return;
			}
			std::cout<<"Connection established. Feel free to chat!\n";
			boost::asio::async_read_until(sock,inbox,'\n',
				boost::bind(&Messenger::handle_read, this, boost::asio::placeholders::error));
			set_connected(true);
		}
		else
		{
			std::cout<<"Someone allready connected to you.\n";
			return;
		}

	}

	void listen()
	{
		acceptor.async_accept(sock,boost::bind(&Messenger::handle_accept,this, boost::asio::placeholders::error));
		connected = false;
	}
	bool get_connected() {return connected;}
	void set_connected(bool val) {connected = val;}

private:
	void handle_accept(const boost::system::error_code &error)
	{
		if(!error)
		{
			acceptor.cancel();
			connected = true;
			std::cout<<"Someone established connection to you! Feel free to chat!\n";
			boost::asio::async_read_until(sock,inbox,'\n',
				boost::bind(&Messenger::handle_read, this, boost::asio::placeholders::error));
		}
		else if(error.value() != boost::asio::error::operation_aborted)
		{
			std::cerr<<"error.message = "<<error.message()<<'\n';
			std::cerr<<"error.code = "<<error.value()<<'\n';
		}

	}

	void handle_read(const boost::system::error_code &error)
	{
		if(!error)
		{
			std::istream is(&inbox);
			std::string msg;
			std::getline(is, msg);
			std::cout<<"->"<<msg<<'\n';
			boost::asio::async_read_until(sock,inbox,'\n',
				boost::bind(&Messenger::handle_read, this, boost::asio::placeholders::error));
		}
		else
		{
			std::cerr<<"error.message = "<<error.message()<<'\n';
			std::cerr<<"error.code = "<<error.value()<<'\n';
		}
	}

	boost::asio::ip::tcp::socket sock;
	boost::asio::ip::tcp::acceptor acceptor;
	boost::asio::streambuf inbox;

	bool connected;
};

int main()
{
	boost::asio::io_service ios;
	Messenger msgr(ios);
	std::string msg;
	std::cout<<"Welcome to my simple p2p Messenger. Please enter ip address which you want connect to or wait until someone would connect to you.\n";
	std::cout<<"To get your ip_address please run \"ipconfig\" in cmd if you are using Windows\n";
	std::cout<<"or \"ifconfig\" if you are using Linux.\n";
	std::cout<<"To exit type \"!q\"\n";
	
	boost::thread th1(boost::bind(&boost::asio::io_service::run,boost::ref(ios)));
	while(true)
	{
		std::getline(std::cin,msg);
		if (msg == "!q")
			break;
		if(msgr.get_connected())
			msgr.send_message(msg);
		else
			msgr.connect_to(msg);
	}
	ios.stop();
	th1.join();
	return 0;
}