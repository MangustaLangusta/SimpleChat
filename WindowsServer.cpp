//class WindowsServer definition


#include <vector>
#include <chrono>
#include <mutex>

#include "WindowsServer.h"

#pragma comment(lib, "Ws2_32.lib")


//min and max versions of sockets (version 2 is for now)
constexpr WORD kSocketsVersion = MAKEWORD(2,2);
/*
const std::string kLocalHostIP = "127.0.0.1";
const unsigned short kDefaultPort = 8000;
*/
#warning DelayTimes not in hardcode
const long kAcceptSocketPeriodMilliseconds = 500000;
const long kRxPeriodMilliseconds = 500000;
const long kTxPeriodMilliseconds = 500000;


WindowsServer::~WindowsServer(){
	//nothing to do
}



//Cleanup is called in each sub-method in case of fail
ServerStatus WindowsServer::StartServer(){
	ServerStatus status;
	
	status = WinSockInitialisation();
	if(!status)
		return kServerERROR;
	
	status = InitialiseSocket();
	if(!status)
		return kServerERROR;
	
	status = BindSocket();
	if(!status)
		return kServerERROR;
	
	status = StartListening();
	if(!status)
		return kServerERROR;
	
	//server_is_active_ flag to be set before start any additional threads
	server_is_active_ = true;
	
	status = StartConnectionsThread();
	if(!status)
		return kServerERROR;
	
	status = StartCommunicationThreads();
	if(!status)
		return kServerERROR;
	
	return kServerOK;
}


ServerStatus WindowsServer::StopServer(){
	server_is_active_ = false;
	Cleanup();
	return kServerOK;
}


ServerStatus WindowsServer::WinSockInitialisation(){
	std::cout << "WinSock initialisation... ";
	int status = WSAStartup(kSocketsVersion, &ws_data_);
	if( status == 0 ){
		std::cout<<"OK"<<std::endl;
		return kServerOK;
	}
	else{
		std::cout	<< "ERROR " << WSAGetLastError()	<< std::endl;
		return kServerERROR;
	}
}


ServerStatus WindowsServer::InitialiseSocket(){
	
	std::cout<< "Socket initialisation... ";	
	own_socket_ = socket(AF_INET, SOCK_STREAM, 0);
	if(own_socket_ != INVALID_SOCKET){
		std::cout << "OK" <<std::endl;
		return kServerOK;
	}
	else {
		std::cout<<"ERROR " << WSAGetLastError() <<std::endl;
		Cleanup();
		return kServerERROR;
	}
}


ServerStatus WindowsServer::BindSocket(){
	std::cout<< "Socket binding... ";
	in_addr ip_to_num;
	
	int error_status;
	error_status = inet_pton( AF_INET, ip_address_.data(), &ip_to_num );
	
	if(error_status <= 0){
		std::cout<< "ERROR during IP translation to special format"<<std::endl;
		return kServerERROR;
	}
	
	sockaddr_in serv_info;
	ZeroMemory(&serv_info, sizeof(serv_info));
	
	serv_info.sin_family = AF_INET;
	serv_info.sin_addr = ip_to_num;
	serv_info.sin_port = htons(port_);
	
	error_status = bind(own_socket_, (sockaddr*)&serv_info, sizeof(serv_info));
	if(error_status != 0){
		std::cout<< "ERROR: "<<WSAGetLastError()<<std::endl;
		Cleanup();
		return kServerERROR;
	}
	
	std::cout<<"OK"<<std::endl;
	return kServerOK;
}


ServerStatus WindowsServer::StartListening(){
	std::cout<< "Start listening... ";
	int error_status;
	error_status = listen(own_socket_, SOMAXCONN);
	if(error_status != 0){
		std::cout<< "ERROR: "<<WSAGetLastError() << std::endl;
		Cleanup();
	}
	std::cout<< "OK"<<std::endl;
	return kServerOK;	
}


ServerStatus WindowsServer::StartConnectionsThread(){
	connections_thread_ = std::make_unique<std::thread>(AcceptConnectionsLoop, this);
	return kServerOK;
}

void WindowsServer::Cleanup(){
	std::cout<<"Connections thread stopped... ";
	if(connections_thread_){
		connections_thread_->join();
		std::cout<<"OK"<<std::endl;
	}
	else{
		std::cout<<"N/A"<<std::endl;
	}
	
	std::cout<<"Communication thread stopped... ";
	if(!active_threads_.empty()){
		for(auto &it : active_threads_){
			it->join();
		}
		std::cout<<"OK"<<std::endl;
	}
	else {
		std::cout<<"N/A"<<std::endl;
	}
	
	closesocket(own_socket_);
	std::cout<<"Socket closed... OK"<<std::endl;
	WSACleanup();
	std::cout<<"Cleanup is done"<<std::endl;
}


void WindowsServer::AcceptConnectionsLoop(WindowsServer* this_instance){
	
	struct timeval accept_period; 
	accept_period.tv_sec = 0;
	accept_period.tv_usec = kAcceptSocketPeriodMilliseconds;
	fd_set listening_socket;
	FD_ZERO(&listening_socket);
	
	while(this_instance->server_is_active_){
		FD_SET(this_instance->own_socket_, &listening_socket);
		int select_result = select(0, &listening_socket, 0, 0, &accept_period);
		
		//time limit expired, next loop
		if(select_result == 0){
			continue;
		}
		
		//error occured
		if(select_result == SOCKET_ERROR){
			std::cout<< "Error during socket select in AcceptConnectionsLoop" <<std::endl;
			std::cout<< "Last error: "<<WSAGetLastError()<<std::endl;
			system("pause");
			continue;
		}
		
		//have request for connection
		
		//lock access to accepted_clients_
		//MUTEX std::cout<<"Access loop wants mutex"<<std::endl;
		std::lock_guard<std::mutex> lock(this_instance->accepted_clients_access_mutex_);
		//MUTEX std::cout<<"Access loop have mutex"<<std::endl;
		
		sockaddr_in client_info;
		int client_info_size = sizeof(client_info);
		
		ZeroMemory(&client_info, client_info_size);
		
		SOCKET new_client = accept(
			this_instance->own_socket_, 
			(sockaddr*)&client_info, 
			&client_info_size);
			
		if(new_client != INVALID_SOCKET){
			this_instance->accepted_clients_.insert(new_client);
			std::cout << "New client connected" << std::endl;
		}
		else {
			std::cout <<"Client detected, but failed to connect. Error: " 
				<< WSAGetLastError()
				<< std::endl;
		}
		//unlock access to accepted_clients_
		//MUTEX std::cout<<"Access loop frees mutex"<<std::endl;
	}
}


ServerStatus WindowsServer::StartCommunicationThreads(){
	active_threads_.emplace_back(std::make_unique<std::thread>(RxLoop, this));
	active_threads_.emplace_back(std::make_unique<std::thread>(TxLoop, this));
	return kServerOK;
}


void WindowsServer::RxLoop(WindowsServer* this_instance){
	
	#warning Do_something_with_rx_buffer_size	
	const short buff_size = 1024;
	std::vector<char> rx_buff(buff_size);
	
	
	struct timeval rx_loop_period; 
	rx_loop_period.tv_sec = 0;
	rx_loop_period.tv_usec = kRxPeriodMilliseconds;
	fd_set connected_sockets;
	FD_ZERO(&connected_sockets);
	
	while(this_instance->server_is_active_){
		for(auto &it : this_instance->accepted_clients_){
			FD_SET(it, &connected_sockets);
		}
		
		if(connected_sockets.fd_count <= 0){
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		
		int select_result = select(0, &connected_sockets, 0, 0, &rx_loop_period);
		
		//no rx requests - next loop
		if(select_result == 0){
			continue;
		}
		
		//some error occured
		if(select_result == INVALID_SOCKET){
			std::cout<<"Error during Rx loop: "<< WSAGetLastError() << std::endl;
			continue;
		}
		
		//process awaiting rx requests
		for(auto &it : this_instance->accepted_clients_){
			
			if( !FD_ISSET(it, &connected_sockets) ){
				continue;
			}
			short pack_size = recv(it, rx_buff.data(), buff_size, 0);

			if((pack_size == SOCKET_ERROR) || (pack_size == 0)){
				//MUTEX std::cout<<"Rx loop want mutex"<< std::endl;
				this_instance->accepted_clients_access_mutex_.lock();
				//MUTEX std::cout<<"Rx loop mutex locked"<< std::endl;
				if(pack_size == 0){
					std::cout<<"Client was disconnected"<<std::endl;
				}
				else{
					std::cout<<"Error during reception. Client will be disconnected."<<std::endl;
				}
				
				this_instance->RemoveClient(it);
				this_instance->accepted_clients_access_mutex_.unlock();
				//MUTEX std::cout<<"Rx loop mutex unlocked"<< std::endl;
				break;
			}
			
			std::string msg = "";
			for(auto &it_buf : rx_buff){
				msg += it_buf;
			}
			std::cout<<"Rx: "<<msg<<std::endl;
			this_instance->ProcessIncomingMessage(it, msg.substr(0, pack_size));
		}
	}
}


void WindowsServer::TxLoop(WindowsServer* this_instance){
	
	struct timeval tx_loop_period; 
	tx_loop_period.tv_sec = 0;
	tx_loop_period.tv_usec = kTxPeriodMilliseconds;
	fd_set connected_sockets;
	FD_ZERO(&connected_sockets);
	
	while(this_instance->server_is_active_){
		
		//MUTEX std::cout<<"Tx loop wants mutex"<<std::endl;
		this_instance->accepted_clients_access_mutex_.lock();
		//MUTEX std::cout<<"Tx loop have mutex"<<std::endl;
		
		for(auto &it : this_instance->accepted_clients_){
			FD_SET(it, &connected_sockets);
		}
		
		//sleep in case of no clients connected
		if(connected_sockets.fd_count <= 0){
			this_instance->accepted_clients_access_mutex_.unlock();
			//MUTEX std::cout<<"Tx loop unlocked mutex"<<std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		
		for(auto &it : this_instance->accepted_clients_){
			
			if( !FD_ISSET(it, &connected_sockets) ){
				continue;
			}
			
			std::optional<std::string> line_to_send = this_instance->OutcomingMessage(it);
			
			if(line_to_send == std::nullopt){
				continue;
			}
			
			//have opened connections and have data to be send
			int length = line_to_send.value().length();
			
			//check for availability of client
			fd_set current_socket;
			FD_ZERO(&current_socket);
			FD_SET(it, &current_socket);
			TIMEVAL null_delay = {0,0};

			int select_result = select(0, 0, &current_socket, 0, &tx_loop_period);
			
			if( select_result == 0){
				continue;
			}
			if( select_result == SOCKET_ERROR ){
				int last_error = WSAGetLastError();
				std::cout<<"TX loop error during checking of client availability. Error: "<<last_error<<std::endl;
				continue;
			}
			
			std::cout<<"Tx loop: OK to transmit"<<std::endl;
				
			int pack_size = send(it, line_to_send.value().data(), length, 0);
			
			if(pack_size != length){
				std::cout<<"Warning! Incomplete sending of TX packet"<<std::endl;
			}
		}	//for (accepted_clients_)
			
		this_instance->accepted_clients_access_mutex_.unlock();
		//MUTEX std::cout<<"Tx loop unlocked mutex"<<std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		
	}	//while (server_is_active_)
}


void WindowsServer::RemoveClient(const SOCKET &client_to_remove){
	auto client_it = accepted_clients_.find(client_to_remove);
	if(client_it == accepted_clients_.end())
		return;
	
	accepted_clients_.erase(client_it);
}


void WindowsServer::ProcessIncomingMessage(SOCKET author, std::string message){
	std::cout<<"Rx: "<<message<<std::endl;
}


std::optional<std::string> WindowsServer::OutcomingMessage(SOCKET receiver){
	return "Check";
}