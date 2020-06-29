#pragma once
#include <map>
#include "protocol_handler.h"

#define CHANNEL_SETTING_BAUD     0x01 // Baud rate change - 32bit unsigned long
#define CHANNEL_SETTING_FLAGS    0x02 // Flag change - 32bit unsigned long
#define CHANNEL_SETTING_PROTOCOL 0x03 // Protocol change - 1 byte (see below)

// Protocol identifiers for sending to Macchina
#define PROTOCOL_ISO15765 0x01
#define PROTOCOL_CAN      0x02
#define PROTOCOL_ISO9141  0x03

#define PROTOCOL_FILTER_BLOCK 0x01 // Block filter for channel
#define PROTOCOL_FILTER_PASS  0x02 // Pass filter for channel
#define PROTOCOL_FILTER_ISO   0x03 // ISO filter (pass filter + Response ID)

/**
	Class that holds data about 1 channel
**/

#define CHANNEL_MAX_FILTERS 10

struct handler_filter {
	uint8_t id;
	uint8_t type;
	PASSTHRU_MSG mask;
	PASSTHRU_MSG filter;
	PASSTHRU_MSG flow;
};

class channel
{
public:
	channel(unsigned long id);
	bool setProtocol(unsigned long ProtocolID);
	bool setFlags(unsigned long Flags);
	bool setBaud(unsigned long Baudrate);
	bool setMacchinaChannel(); // Sets the channel up on Macchina
	int sendPayload(PASSTHRU_MSG* msg);
	int setFilter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID);
	int remove_filter(unsigned long filterID);
	void removeChannel();
private:
	protocol_handler* handler = nullptr;
	uint8_t macchinaProtocolID;
	handler_filter* filters[CHANNEL_MAX_FILTERS] = { nullptr };
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
	int setFilter(unsigned long channel_id, unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID);
	int remove_filter(unsigned long channel_id,  unsigned long filterID);
	int send_payload(unsigned long channel_id, PASSTHRU_MSG *pMsg, unsigned long* pNumMsgs, unsigned long timeout);
	channel* getChannelWithID(unsigned long id);
	unsigned long addChannel(unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate);
	int removeChannel(unsigned long channelid);
};

extern channel_group channels;

