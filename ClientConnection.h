//class ClientConnection

#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <optional>
#include "IConnection.h"


class ClientConnection : public IConnection{
private:
public:
	~ClientConnection();
	void ReceiveData(std::string rx_data) override;
	std::optional<std::string> GetTxData() override;
	
	std::optional<std::string> NextIncomingMessage() override;
	void Send(std::string msg_to_send) override;
private:	
};

#endif	//SIMPLE_CONNECTION_H