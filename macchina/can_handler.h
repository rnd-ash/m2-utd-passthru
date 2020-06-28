#pragma once

#ifndef CANBUS_H
#define CANBUS_H

#include <due_can.h>

#define CAN0_LED DS3 // CAN 0 LED - On if send or receive data
#define CAN1_LED DS4 // CAN 1 LED - On if send or receive data

class canbus_handler {
public:
    canbus_handler(uint8_t id, uint32_t baud);
    void setFilter(uint32_t canid, uint32_t mask, bool isExtended);
    void transmit(CAN_FRAME *f);
    bool read(CAN_FRAME *f);
private:
    CANRaw* can;
    uint8_t actLED;
};

#endif