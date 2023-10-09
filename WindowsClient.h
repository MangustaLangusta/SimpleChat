//class WindowsClient

#ifndef WINDOWS_CLIENT_H 
#define WINDOWS_CLIENT_H

#include <optional>
#include <iostream>


#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <memory>

#include "IClient.h"

class WindowsClient : public IClient{
private:

	ClientStatus WinSockInitialisation();
	ClientStatus InitialiseSocket();
	ClientStatus ConnectToServer(std::string ip_address, const unsigned short port);
	ClientStatus StartConnectionThreads();
	static void RxLoop(WindowsClient* this_instance);
	static void TxLoop(WindowsClient* this_instance);

	void Cleanup();
public:
	WindowsClient(std::shared_ptr<IConnection> client_service);
	~WindowsClient();
	ClientStatus Connect(const std::string &ip_address, const unsigned short port) override;
	ClientStatus Disconnect() override;
	virtual void SendLine(std::string &str) override;
private:
	WSADATA ws_data_;
	SOCKET own_socket_;
	std::unique_ptr<std::thread> rx_thread_;
	std::unique_ptr<std::thread> tx_thread_;
};

#endif