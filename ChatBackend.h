//class ChatBackend

#ifndef CHAT_BACKEND_H
#define CHAT_BACKEND_H

#include <optional>
#include <iostream>
#include <mutex>
#include <string>
#include <any>
#include <map>
#include <list>

typedef int ClientID;

class ChatMessage{
private:
public:
	ChatMessage() = delete;
	ChatMessage(const std::string receiver, const std::string author, const std::string message);
	
	std::string Receiver();
	std::string Author();
	std::string Message();
	
	~ChatMessage() = default;
private:
	std::string receiver_;
	std::string author_;
	std::string message_;
};


class ClientInfoContainer{
private:
public:
	ClientInfoContainer() { std::cout << "client info container made" << std::endl; }
	ClientInfoContainer(const ClientID id, const std::string name);
	
	~ClientInfoContainer() = default;
	
	ClientID ID();
	std::string Name();
	std::optional<std::string> GetPendingMessage();
	void ReceivedMessage(const ChatMessage &new_message);
	
private:
	ClientID id_;
	std::string name_;
	std::list<ChatMessage> pending_messages_;
};


class ChatBackend{
private:
	std::optional<ChatMessage> ParseLineToMessage(std::string author, std::string line);
	void NewClient(ClientID id, std::string name);
	void MessageFromUnknownClient(ClientID author, std::string rx_data);
public:

	std::optional<std::string> GetTxData(const ClientID receiver);
	void Received(ClientID author, std::string rx_data);
	
private:	
	//any represents possible sockets id's. ClientID is internal handler to client
	std::map<ClientID, std::string> name_by_id_map_;
	std::map<std::string, ClientID> id_by_name_map_;
	
	std::map<ClientID, ClientInfoContainer> client_by_id_map_;
	
	std::mutex data_mutex_;
};

#endif	//CHAT_BACKEND_H