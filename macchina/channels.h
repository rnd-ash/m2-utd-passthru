#pragma once
#ifndef CHANNELS_H
#define CHANNELS_H

#include "handlers.h"
#include <stdint.h>

// Protocol identifiers for sending to Macchina
#define PROTOCOL_ISO15765 0x01
#define PROTOCOL_CAN      0x02
#define PROTOCOL_ISO9141  0x03

class channel {
public:
    channel(uint8_t id, uint8_t protocol, unsigned long baudRate);
    void kill_channel();
    void update();
    uint8_t getID();
    void transmit_data(uint16_t len, uint8_t* data);
private:
    handler* protocol_handler;
    uint8_t id;
};

#endif