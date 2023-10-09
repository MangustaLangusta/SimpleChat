// class ChatServer definition

#include "ChatServer.h"

void ChatServer::ProcessIncomingMessage(SOCKET author, std::string message){
	
	Received(author, message);
	
}


std::optional<std::string> ChatServer::OutcomingMessage(SOCKET receiver){

	return GetTxData(receiver);

}