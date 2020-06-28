#pragma once
#include "j2534_v0404.h"

class protocol_handler
{
public:
	explicit protocol_handler(unsigned long channelID);
	void setFlags(unsigned long flags);
	void setBaud(unsigned long baud);
protected:
	unsigned long id;
	unsigned long baud;
	unsigned long flags;
	unsigned long channelid;
};

class iso9141_handler : public protocol_handler {
public:
	iso9141_handler(unsigned long channelID);
};

class iso15765_handler : public protocol_handler {
public:
	iso15765_handler(unsigned long channelID);
};

class can_handler : public protocol_handler {
public:
	can_handler(unsigned long channelID);
};

