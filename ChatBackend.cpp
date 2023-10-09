//class ChatBackend definition

#include "ChatBackend.h"

	//==================================== CHAT MESSAGE ===================================//
ChatMessage::ChatMessage(const std::string receiver, const std::string author, const std::string message){
	receiver_ = receiver;
	author_ = author;
	message_ = message;
	std::cout<<"ChatMessage created. receiver: " << receiver_ << "; author: " << author_ << "; message: " << message_ << std::endl;
}


std::string ChatMessage::Receiver(){
	return receiver_;	
}


std::string ChatMessage::Author(){
	return author_;
}


std::string ChatMessage::Message(){
	return message_;
}

	//=============================== CLIENT INFO CONTAINER =============================//
ClientInfoContainer::ClientInfoContainer(const ClientID id, const std::string name){
	id_ = id;
	name_ = name;
	pending_messages_ = {};
	std::cout<<"ClientInfoContainer created. id = " << id_ << "; name = " << name_ << std::endl;
}

	
ClientID ClientInfoContainer::ID(){
	return id_;
}


std::string ClientInfoContainer::Name(){
	return name_;
}


std::optional<std::string> ClientInfoContainer::GetPendingMessage(){
	if(pending_messages_.empty()){
		return std::nullopt;
	}
	ChatMessage message = std::move(pending_messages_.front());
	pending_messages_.pop_front();
	
	std::string result = std::any_cast<std::string>(message.Author()) + ":\n" + message.Message() + "\n";
	return std::move(result);
}


void ClientInfoContainer::ReceivedMessage(const ChatMessage &new_message){
	pending_messages_.emplace_back(std::move(new_message));
}
	

//=============================== CHAT BACKEND =============================//
std::optional<ChatMessage> ChatBackend::ParseLineToMessage(std::string author, std::string line){
	std::cout<<"ParseLineToMessage. line: "<<line<<"; author = "<<author<<std::endl;
	std::size_t delim_pos = line.find(" ");
	if(delim_pos == std::string::npos){
		return std::nullopt;
	}
	std::string receiver_name = line.substr(0, delim_pos);
	std::string message = line.substr(delim_pos + 1, std::string::npos);
	
	ChatMessage result(receiver_name, author, std::move(message));
	return std::move(result);
}


void ChatBackend::NewClient(ClientID id, std::string name){
	std::cout<<"NewClient. id: "<<id<<"; name = "<<name<<std::endl;
	name_by_id_map_[id] = name;
	id_by_name_map_[name] = id;
	client_by_id_map_[id] = std::move( ClientInfoContainer( id, name ) );
}



void ChatBackend::MessageFromUnknownClient(ClientID author, std::string rx_data){
	std::cout<<"MessageFromUnknownClient. id: "<<author<<"; rx_data = "<<rx_data<<std::endl;
	std::optional<ChatMessage> message = ParseLineToMessage( "", std::move(rx_data + " ") );
	if(message == std::nullopt)
		return;
	
	std::string receiver_name = message.value().Receiver() ;
	
	NewClient(author, receiver_name);	//new client name will be in message field of receiver
}


std::optional<std::string> ChatBackend::GetTxData(const ClientID receiver){
	//std::cout<<"GetTxData. receiver: "<<receiver<<std::endl;
	std::lock_guard<std::mutex> lock(data_mutex_);
	
	auto client_id = client_by_id_map_.find(receiver);
	if(client_id == client_by_id_map_.end())
		return std::nullopt;

	return client_id->second.GetPendingMessage();
}


void ChatBackend::Received(ClientID author, std::string rx_data){
	std::cout<<"Received. author id: "<<author<<"; data: "<< rx_data<<std::endl;
	std::lock_guard<std::mutex> lock(data_mutex_);
	
	auto name_it = name_by_id_map_.find(author);
	
	//if author is not registered yet. Need to register
	if(name_it == name_by_id_map_.end()){
		MessageFromUnknownClient(author, std::move(rx_data));
		return;
	}
	
	std::string author_name = name_it->second;
	std::optional<ChatMessage> message = ParseLineToMessage( author_name, std::move(rx_data) );
	if(message == std::nullopt)
		return;
		
	auto receiver_it = id_by_name_map_.find(message.value().Receiver());
	if(receiver_it == id_by_name_map_.end())
		return;
	
	ClientID receiver_id = receiver_it->second;
	client_by_id_map_[receiver_id].ReceivedMessage(message.value());	
}
	/*
	//any represents possible sockets id's. ClientID is internal handler to client
	std::map<ClientId, std::string> name_by_id_map_;
	std::map<std::string, ClientID> id_by_name_map_;
	
	std::map<ClientID, ClientInfoContainer> client_by_id_map_;
	
	std::mutex data_mutex_;*/