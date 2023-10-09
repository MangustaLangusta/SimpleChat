//class WindowsServer definition

#include "WindowsClient.h"

#include <vector>


#pragma comment(lib, "Ws2_32.lib")


//min and max versions of sockets (version 2 is for now)
constexpr WORD kSocketsVersion = MAKEWORD(2,2);

const long kRxPeriodMilliseconds = 500000;
const long kTxPeriodMilliseconds = 500000;

WindowsClient::WindowsClient(std::shared_ptr<IConnection> client_service) 
	: IClient(client_service){
	//nothing to do
}


WindowsClient::~WindowsClient(){
	connected_ = false;
	Cleanup();
}


void WindowsClient::Cleanup(){
	
	connected_ = false;
	
	std::cout<<"Rx thread stopped... ";
	if(rx_thread_){
		rx_thread_->join();
		rx_thread_ = nullptr;
		std::cout<<"OK"<<std::endl;
	}
	else{
		std::cout<<"N/A"<<std::endl;
	}
	
	std::cout<<"Tx thread stopped... ";
	if(tx_thread_){
		tx_thread_->join();
		tx_thread_ = nullptr;
		std::cout<<"OK"<<std::endl;
	}
	else{
		std::cout<<"N/A"<<std::endl;
	}
	
	closesocket(own_socket_);
	std::cout<<"Socket closed... OK"<<std::endl;
	WSACleanup();
	std::cout<<"Cleanup is done"<<std::endl;
}


ClientStatus WindowsClient::Connect(const std::string &ip_address, const unsigned short port) {
	
	if(connected_){
		Cleanup();
		connected_ = false;
	}
	
	ClientStatus status;
	
	status = WinSockInitialisation();
	if(!status)
		return kClientERROR;
	
	status = InitialiseSocket();
	if(!status)
		return kClientERROR;
	
	status = ConnectToServer(ip_address, port);
	if(!status)
		return kClientERROR;
	
	connected_ = true;
	
	StartConnectionThreads();
	
	return kClientOK;
}


ClientStatus WindowsClient::WinSockInitialisation(){
	std::cout << "WinSock initialisation... ";
	int status = WSAStartup(kSocketsVersion, &ws_data_);
	if( status == 0 ){
		std::cout<<"OK"<<std::endl;
		return kClientOK;
	}
	else{
		std::cout	<< "ERROR " << WSAGetLastError()	<< std::endl;
		return kClientERROR;
	}
}


ClientStatus WindowsClient::InitialiseSocket(){
	
	std::cout<< "Socket initialisation... ";	
	own_socket_ = socket(AF_INET, SOCK_STREAM, 0);
	if(own_socket_ != INVALID_SOCKET){
		std::cout << "OK" <<std::endl;
		return kClientOK;
	}
	else {
		std::cout<<"ERROR " << WSAGetLastError() <<std::endl;
		Cleanup();
		return kClientERROR;
	}
}


ClientStatus WindowsClient::StartConnectionThreads(){
	rx_thread_ = std::make_unique<std::thread>(RxLoop, this);
	tx_thread_ = std::make_unique<std::thread>(TxLoop, this);
	return kClientOK;
}


void WindowsClient::RxLoop(WindowsClient* this_instance){
	#warning Do_something_with_rx_buffer_size	
	const short buff_size = 1024;
	std::vector<char> rx_buff(buff_size);
	
	
	struct timeval rx_loop_period; 
	rx_loop_period.tv_sec = 0;
	rx_loop_period.tv_usec = kRxPeriodMilliseconds;
	fd_set server_socket;
	FD_ZERO(&server_socket);
	TIMEVAL null_delay = {0, 0};
	
	while(this_instance->connected_){
		
		FD_SET(this_instance->own_socket_, &server_socket);
		int select_result = select(0, &server_socket, 0, 0, &rx_loop_period);
		
		//no rx requests - next loop
		if(select_result == 0){
			continue;
		}
		
		//some error occured
		if(select_result == SOCKET_ERROR){
			std::cout<<"Error in Rx loop: "<< WSAGetLastError() << std::endl;
			std::cout<<"Connection will be stopped"<<std::endl;
			this_instance->connected_ = false;
			continue;
		}
		
		//process awaiting rx request
		short pack_size = recv(this_instance->own_socket_, rx_buff.data(), buff_size, 0);
		
			if((pack_size == SOCKET_ERROR) || (pack_size == 0)){
			
			std::cout<<"Server was disconnected"<<std::endl;

			this_instance->connected_ = false;
			continue;
		}
		
		
		
		std::string msg = "";
		for(auto &it_buf : rx_buff){
			msg += it_buf;
		}

		this_instance->client_service_->ReceiveData(msg);
	}
}


void WindowsClient::TxLoop(WindowsClient* this_instance){
	#warning Do_something_with_tx_buffer_size	
	const short buff_size = 1024;
	std::vector<char> tx_buff(buff_size);
	
	
	struct timeval tx_loop_period; 
	tx_loop_period.tv_sec = 0;
	tx_loop_period.tv_usec = kTxPeriodMilliseconds;
	fd_set server_socket;
	FD_ZERO(&server_socket);
	
	while(this_instance->connected_){

		FD_SET(this_instance->own_socket_, &server_socket);
		
		int select_result = select(0, 0, &server_socket, 0, &tx_loop_period);
		
		//no rx requests - next loop
		if(select_result == 0){
			continue;
		}
		
		//some error occured
		if(select_result == INVALID_SOCKET){
			std::cout<<"Error during Tx loop: "<< WSAGetLastError() << std::endl;
			continue;
		}
		
		std::optional<std::string> line_to_send = this_instance->client_service_->GetTxData();
		
		if(!line_to_send){
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		
		//have connection established and have data to be send
		int length = line_to_send.value().length();
		
		int pack_size = send(this_instance->own_socket_, line_to_send.value().data(), length, 0);
		
		if(pack_size != length){
			std::cout<<"Warning! Incomplete sending of TX packet"<<std::endl;
		}
	}	//while connected_
}


ClientStatus WindowsClient::ConnectToServer(std::string ip_address, const unsigned short port){
	
	std::cout<< "Connection to server " << ip_address << " : " << port <<"... ";
	
	in_addr ip_to_num;
	int error_status;
	
	error_status = inet_pton( AF_INET, ip_address.data(), &ip_to_num );

	if(error_status <= 0){
		std::cout<< std::endl << "ERROR during IP translation to special format"<<std::endl;
		Cleanup();
		return kClientERROR;
	}
	
	sockaddr_in serv_info;
	ZeroMemory(&serv_info, sizeof(serv_info));
	
	serv_info.sin_family = AF_INET;
	serv_info.sin_addr = ip_to_num;
	serv_info.sin_port = htons(port);

	error_status = connect(own_socket_, (sockaddr*)&serv_info, sizeof(serv_info));
	
	if (error_status != 0) {
		std::cout << "FAILED. Error # " << WSAGetLastError() << std::endl;
		Cleanup();
		return kClientERROR;
	}
	else{
		std::cout << "OK" << std::endl;
	}
	return kClientOK;
}


ClientStatus WindowsClient::Disconnect(){
	if(!connected_)
		return kClientOK;
	connected_ = false;
	Cleanup();
	return kClientOK;
}


void WindowsClient::SendLine(std::string &str){
	if(!connected_){
		std::cout<<"Warning! Client is not connected to server"<<std::endl;
	}
	client_service_->Send(str);
}
