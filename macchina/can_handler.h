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

#ifndef CANBUS_H
#define CANBUS_H

#include "variant.h"
#include "due_can.h"

#define CAN0_LED DS3 // CAN 0 LED - On if send or receive data
#define CAN1_LED DS4 // CAN 1 LED - On if send or receive data

class canbus_handler {
public:
    canbus_handler(CANRaw* can, uint8_t led_pin);
    void setFilter(uint32_t canid, uint32_t mask, bool isExtended);
    void transmit(CAN_FRAME f);
    bool read(CAN_FRAME* f);
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