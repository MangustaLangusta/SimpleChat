
#include <string>
#include <iostream>

#include "ChatServer.h"

const std::string kLocalhostIP = "127.0.0.1";
const unsigned short kDefaultPort = 8000;


int main(){

	ChatServer my_server(kLocalhostIP, kDefaultPort);
	
	my_server.StartServer();
	
	system("pause");
	std::cout<<"now we stop our server"<<std::endl;
	my_server.StopServer();
	return 0;
}

//g++ server.cpp SimpleConnection.cpp WindowsServer.cpp -lWs2_32