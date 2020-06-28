#pragma once

#ifndef HANDLERS_H_
#define HANDLERS_H_

#include "can_handler.h"

/**
 * Handlers for various protocol - Base class
 */
class handler {
public:
    handler(unsigned long baud);
    virtual void update() = 0;
    virtual void destroy() = 0;
    virtual void transmit(uint8_t* args, uint16_t len) = 0;
private:

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
private:
    canbus_handler *can_handle;
};

#endif