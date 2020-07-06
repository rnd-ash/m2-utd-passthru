#pragma once

#ifndef HANDLERS_H_
#define HANDLERS_H_

#include "can_handler.h"

#define MAX_FILTERS_PER_HANDLER 10
#define ISO15765_FF_INDICATOR 0xFF // ISO15765 First frame indication

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
    unsigned long nextSend = millis();
    
};

#endif