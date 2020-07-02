#pragma once

#ifndef CANBUS_H
#define CANBUS_H

#include "variant.h"
#include "due_can.h"

#define CAN0_LED DS3 // CAN 0 LED - On if send or receive data
#define CAN1_LED DS4 // CAN 1 LED - On if send or receive data

class canbus_handler {
public:
    canbus_handler(CANRaw* can);
    void setFilter(uint32_t canid, uint32_t mask, bool isExtended);
    void transmit(CAN_FRAME f);
    bool read(CAN_FRAME f);
    void unlock();
    void lock(uint32_t baud);
    bool isFree();
private:
    CANRaw *can;
    uint8_t actLED;
    bool inUse = false;
};

extern canbus_handler ch0;
extern canbus_handler ch1;

#endif