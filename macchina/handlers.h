#pragma once

#ifndef HANDLERS_H_
#define HANDLERS_H_

#include "can_handler.h"

#define MAX_FILTERS_PER_HANDLER 10

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
    virtual void update() = 0;
    virtual void destroy() = 0;
    virtual void transmit(uint8_t* args, uint16_t len) = 0;
    void add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp);
    void destroy_filter(uint8_t id);
private:
    handler_filter* filters[MAX_FILTERS_PER_HANDLER] = { nullptr };
};

/**
 * CAN protocol handler
 */
class can_handler : public handler {
public:
    can_handler(unsigned long baud);
    void update();
    void destroy();
    void transmit(uint8_t* args, uint16_t len);
    void add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp);
private:
    canbus_handler *can_handle;
};

/**
 * ISO9141 handler for K-Line
 */
class iso9141_handler : public handler {
public:
    iso9141_handler(unsigned long baud);
    void update();
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
    iso15765_handler(unsigned long baud);
    void update();
    void destroy();
    void transmit(uint8_t* args, uint16_t len);
    void add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp);
private:
    canbus_handler *can_handle;
};

#endif