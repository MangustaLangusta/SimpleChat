//class IClient
#ifndef ICLIENT_H
#define ICLIENT_H

#include <memory>
#include <string>

#include "IConnection.h"

typedef enum{
	kClientOK = true,
	kClientERROR = false
}	ClientStatus;

class IClient{
protected:
public:
	IClient(std::shared_ptr<IConnection> client_service) 
		: client_service_{std::move(client_service)} { }
	virtual ~IClient() {}
	virtual ClientStatus Connect(const std::string &ip_address, const unsigned short port) = 0;
	virtual ClientStatus Disconnect() = 0;
	virtual void SendLine(std::string &str) = 0;
	
protected:
	std::string ip_address_;
	unsigned short port_;
	bool connected_ = false;
	std::shared_ptr<IConnection> client_service_;
};

#endif //ICLIENT_H