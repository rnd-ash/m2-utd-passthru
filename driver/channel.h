#pragma once
#include <map>
#include <queue>
#include <tuple>
#include "protocol_handler.h"
#include "usbcomm.h"

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

/// <summary>
/// Struct for storing filter data
/// </summary>
struct handler_filter {
	uint8_t id; // Unique ID per channel
	uint8_t type; // Type of filter
	PASSTHRU_MSG mask; // Mask
	PASSTHRU_MSG filter; // Filter
	PASSTHRU_MSG flow; // Flow control CAN ID for ISO15765
};

class channel
{
public:
	channel(unsigned long id);
	int setProtocol(unsigned long ProtocolID);
	int setFlags(unsigned long Flags);
	int setBaud(unsigned long Baudrate);
	int setMacchinaChannel(); // Sets the channel up on Macchina
	int sendPayload(PASSTHRU_MSG* msg);
	int setFilter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID);
	int remove_filter(unsigned long filterID);
	int removeChannel();
	void recvData(uint8_t* m, uint16_t len);
	int requestData(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
private:
	std::queue<PASSTHRU_MSG> msg_queue;
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
	std::tuple<int, unsigned long> addChannel(unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate);
	int removeChannel(unsigned long channelid);
	void recvPayload(PCMSG* m);
	int requestChannelData(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
};

extern channel_group channels;

