#pragma once
#include <map>
#include "protocol_handler.h"

/**
	Class that holds data about 1 channel
**/
class channel
{
public:
	channel(unsigned long id);
	bool setProtocol(unsigned long ProtocolID);
	bool setFlags(unsigned long Flags);
	bool setBaud(unsigned long Baudrate);
	void removeChannel();
private:
	protocol_handler* handler = nullptr;
	unsigned long id;
};


/**
	Class that holds a group of channels
**/
class channel_group {
#define MAX_CHANNELS 10
private:
	std::map<unsigned long, channel> channels;
	unsigned long getFreeChannelID();
	bool used[MAX_CHANNELS] = { false };
public:
	channel* getChannelWithID(unsigned long id);
	unsigned long addChannel(unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate);
	int removeChannel(unsigned long channelid);
};

extern channel_group channels;

