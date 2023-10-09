//class ChatServer

#include "WindowsServer.h"
#include "ChatBackend.h"


class ChatServer : public WindowsServer, private ChatBackend{
private:
	void ProcessIncomingMessage(SOCKET author, std::string message) override;
	std::optional<std::string> OutcomingMessage(SOCKET receiver) override;

public: 
	ChatServer() = delete;
	ChatServer(const std::string &ip_address, const unsigned short port) : WindowsServer(ip_address, port), ChatBackend() {}
	~ChatServer() = default;
};
