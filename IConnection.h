//Interface IConnection

#ifndef ICONNECTION_H
#define ICONNECTION_H

#include <list>
#include <string>
#include <optional>


class IConnection{
private:

public:
	virtual ~IConnection() = default;
	
	virtual void ReceiveData(std::string rx_data) = 0;
	virtual std::optional<std::string> GetTxData() = 0;
	
	virtual std::optional<std::string> NextIncomingMessage() = 0;
	virtual void Send(std::string msg_to_send) = 0;
protected:
	std::list<std::string> outbox_;
	std::list<std::string> inbox_;
};

#endif //ICONNECTION_H