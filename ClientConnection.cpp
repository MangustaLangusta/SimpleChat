//class ClientConnection definition

#include "ClientConnection.h"

#include <string>
#include <iostream>


ClientConnection::~ClientConnection(){
	//nothing to do
}
	
	

void ClientConnection::ReceiveData(std::string rx_data){
	std::cout<<rx_data<<std::endl;
}


std::optional<std::string> ClientConnection::GetTxData(){
	if(outbox_.empty())
		return std::nullopt;
	std::string result = std::move(outbox_.front());
	outbox_.pop_front();
	
	return std::move(result);
}


std::optional<std::string> ClientConnection::NextIncomingMessage(){
	if(inbox_.empty())
		return std::nullopt;
	std::string result = std::move(inbox_.front());
	inbox_.pop_front();
	return std::move(result);
}


void ClientConnection::Send(std::string msg_to_send){
	outbox_.emplace_back(std::move(msg_to_send));
}