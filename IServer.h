//class IServer

#ifndef ISERVER_H
#define ISERVER_H

#include "IConnection.h"

typedef enum{
	kServerOK = true,
	kServerERROR = false
}	ServerStatus;

class IServer{
private:
protected:
public:
	IServer(const std::string &ip_address, const unsigned short port)	: ip_address_{ip_address}, port_{port}	{ }
				
	virtual ~IServer() {}
	virtual ServerStatus StartServer() = 0;
	virtual ServerStatus StopServer() = 0;
	
protected:
	std::string ip_address_;
	unsigned short port_;
	bool server_is_active_ = false;
};

#endif //ISERVER_H