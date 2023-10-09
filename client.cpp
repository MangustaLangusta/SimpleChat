
#include <string>
#include <iostream>

#include "WindowsClient.h"
#include "ClientConnection.h"

const std::string kLocalhostIP = "127.0.0.1";
const unsigned short kDefaultPort = 8000;


int main(){
	
	WindowsClient my_client( std::make_shared<ClientConnection>() );
	
	if( my_client.Connect(kLocalhostIP, kDefaultPort) == kClientERROR )
		return 0;
	
	
	
	std::string my_str = "";
	std::cout<<"enter line to send (or exit to stop)"<<std::endl;
	while(my_str != "exit"){
	std::getline(std::cin, my_str);
	my_client.SendLine(my_str);
	}
	
	my_client.Disconnect();
	return 0;
}

//g++ client.cpp WindowsClient.cpp -lWs2_32 -oclient.exe
