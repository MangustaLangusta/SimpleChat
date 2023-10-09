//class WindowsServer

#ifndef WINDOWS_SERVER_H 
#define WINDOWS_SERVER_H

#include <optional>
#include <iostream>
#include <set>
#include <thread>
#include <memory>
#include <list>
#include <mutex>


#include <WinSock2.h>
#include <WS2tcpip.h>

#include "IServer.h"



class WindowsServer : public IServer{
private:
	ServerStatus WinSockInitialisation();
	ServerStatus InitialiseSocket();
	ServerStatus BindSocket();
	ServerStatus StartListening();
	ServerStatus StartConnectionsThread();
	ServerStatus StartCommunicationThreads();
	static void AcceptConnectionsLoop(WindowsServer* this_instance);
	static void RxLoop(WindowsServer* this_instance);
	static void TxLoop(WindowsServer* this_instance);
	void Cleanup();
	
	void RemoveClient(const SOCKET &client_to_remove);
	
protected: 
	virtual void ProcessIncomingMessage(SOCKET author, std::string message);
	virtual std::optional<std::string> OutcomingMessage(SOCKET receiver);
	
public:
	WindowsServer(const std::string &ip_address, const unsigned short port) : IServer(ip_address, port) {}
	~WindowsServer();
	ServerStatus StartServer() override;
	ServerStatus StopServer() override;
private:
	WSADATA ws_data_;
	SOCKET own_socket_;
	std::unique_ptr<std::thread> connections_thread_;
	std::list<std::unique_ptr<std::thread>> active_threads_;
	std::mutex accepted_clients_access_mutex_;
	std::set<SOCKET> accepted_clients_;
};

#endif