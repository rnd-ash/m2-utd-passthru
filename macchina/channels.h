#pragma once
#ifndef CHANNELS_H
#define CHANNELS_H

#include <stdint.h>

class channel {
public:
    channel(uint8_t id);
    void kill_channel();
    void update();
    uint8_t getID();
private:
    uint8_t id;
};

/**
 * Handlers for various protocol - Base class
 */
class handler {

};

/**
 * CAN protocol handler
 */
class can_handler {

};

/**
 * ISO9141 handler for K-Line
 */
class iso9141_handler {

};

/**
 * ISO15765 handler for Large CAN payloads
 */
class iso15765_handler {

};

#endif