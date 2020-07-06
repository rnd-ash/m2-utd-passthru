/*
**
** Copyright (C) 2020 Ashcon Mohseninia
** Author: Ashcon Mohseninia <ashcon50@gmail.com>
**
** This library is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at
** your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, <http://www.gnu.org/licenses/>.
**
*/

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