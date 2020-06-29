#pragma once
#ifndef CHANNELS_H
#define CHANNELS_H

#include "handlers.h"
#include <stdint.h>

// Protocol identifiers for sending to Macchina
#define PROTOCOL_ISO15765 0x01
#define PROTOCOL_CAN      0x02
#define PROTOCOL_ISO9141  0x03

#define PROTOCOL_FILTER_BLOCK 0x01 // Block filter for channel
#define PROTOCOL_FILTER_PASS  0x02 // Pass filter for channel
#define PROTOCOL_FILTER_FLOW  0x03 // ISO 15765 filter (pass filter + Response ID)

class channel {
public:
    channel(uint8_t id, uint8_t protocol, unsigned long baudRate);
    void kill_channel();
    void update();
    uint8_t getID();
    void transmit_data(uint16_t len, uint8_t* data);
    void set_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp);
    void remove_filter(uint8_t id);
private:
    handler* protocol_handler;
    uint8_t id;
};

#endif