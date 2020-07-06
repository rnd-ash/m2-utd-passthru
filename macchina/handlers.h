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

#ifndef HANDLERS_H_
#define HANDLERS_H_

#include "can_handler.h"

#define MAX_FILTERS_PER_HANDLER 10
#define ISO15765_FF_INDICATOR 0xFF // ISO15765 First frame indication
#define ISO15765_SD_INDICATOR 0xAA // ISO15765 Indication of complete transmission

struct handler_filter {
    uint8_t id;
    uint8_t type;
    uint32_t mask;
    uint32_t filter;
    uint32_t flow;
};

/**
 * Handlers for various protocol - Base class
 */
class handler {
public:
    handler(unsigned long baud);
    virtual bool update();
    virtual void destroy();
    virtual void transmit(uint8_t* args, uint16_t len) = 0;
    virtual void add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp);
    virtual void destroy_filter(uint8_t id);
    uint8_t* getBuf();
    uint8_t getBufSize();
private:
    handler_filter* filters[MAX_FILTERS_PER_HANDLER] = { nullptr };
protected:
    uint32_t getFilterResponseID(uint32_t rxID);
    uint8_t* buf;
    uint8_t buflen;
    virtual bool getData() = 0;
};

/**
 * CAN protocol handler
 */
class can_handler : public handler {
public:
    can_handler(unsigned long baud);
    bool getData();
    void destroy();
    void transmit(uint8_t* args, uint16_t len);
    void add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp);
private:
    CAN_FRAME lastFrame;
    canbus_handler *can_handle;
};

/**
 * ISO9141 handler for K-Line
 */
class iso9141_handler : public handler {
public:
    iso9141_handler(unsigned long baud);
    bool getData();
    void destroy();
    void transmit(uint8_t* args, uint16_t len);
    void add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp);
private:
    can_handler *can_handle;
};


#define MAX_BLOCK_SIZE_RX 8

/**
 * ISO15765 handler for Large CAN payloads
 */
class iso15765_handler : public handler {
public:
    iso15765_handler(unsigned long baud, uint8_t chanid);
    bool getData();
    void destroy();
    void transmit(uint8_t* args, uint16_t len);
    void add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp);
    void sendFF(uint32_t canid);
private:
    uint8_t channel_id; // Used for FF indications
    uint16_t bufWritePos = 0;
    CAN_FRAME lastFrame;
    canbus_handler *can_handle;

    // For ISO 15765 Sending
    unsigned long tx_last_send_time = millis();
    bool isSending = false;
    bool clearToSend = false;
    bool isReceiving = false;
    uint8_t rx_count; // When asking to generate a new FC message
    uint8_t* tx_buffer;
    uint8_t  tx_buffer_size;
    uint8_t  tx_buffer_pos;
    uint8_t  tx_packet_id;
    uint8_t  tx_packets_sent;
    uint8_t  tx_bs; // Block size
    uint8_t  tx_sep; // Seperation time
    CAN_FRAME tx_frame;
    void send_buffer();
    void send_flow_control();
};

#endif