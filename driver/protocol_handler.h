#pragma once
#include <queue>
#include "j2534_v0404.h"

class protocol_handler
{
public:
	explicit protocol_handler(unsigned long channelID);
	void setFlags(unsigned long flags);
	void setBaud(unsigned long baud);
	unsigned long getBaud();
	virtual void recvData(uint8_t* m, uint16_t len) = 0;
	int requestData(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
protected:
	std::queue<PASSTHRU_MSG> msg_queue;
	unsigned long baud;
	unsigned long flags;
	unsigned long channelid;
};

class iso9141_handler : public protocol_handler {
public:
	iso9141_handler(unsigned long channelID);
	void recvData(uint8_t* m, uint16_t len);
};

class iso15765_handler : public protocol_handler {
public:
	iso15765_handler(unsigned long channelID);
	void recvData(uint8_t* m, uint16_t len);
};

class can_handler : public protocol_handler {
public:
	can_handler(unsigned long channelID);
	void recvData(uint8_t* m, uint16_t len);
};

