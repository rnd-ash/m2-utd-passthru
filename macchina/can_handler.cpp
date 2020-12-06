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

#include "can_handler.h"
#include "pc_comm.h"

// Try to init CAN interface on one of the 2 avaliable built in interfaces
canbus_handler::canbus_handler(CANRaw* can, uint8_t led_pin) {
    if (!can) {
        PCCOMM::logToSerial("CONSTRUCTOR - WTF Can is null!?");
        return;
    }
    this->can = can;
    this->actLED = led_pin;
}

// Is this interface handler free to be claimed?
bool canbus_handler::isFree() {
    return !this->inUse;
}

// Set a filter on one of the Rx mailboxes
void canbus_handler::setFilter(uint32_t canid, uint32_t mask, bool isExtended) {
    char buf[40] = {0x00};
    sprintf(buf, "Setting Rx Filters - MASK: 0x%04X, Filter: 0x%04X", mask, canid, 40);
    PCCOMM::logToSerial(buf);
    for (int i = 0; i < 7; i++) {
        this->can->setRXFilter(canid, mask, isExtended);
    }
}

// Transmits a frame on the bus
void canbus_handler::transmit(CAN_FRAME f) {
    if (!this->can) {
        PCCOMM::logToSerial("TRANSMIT - WTF Can is null!?");
        return;
    }
    digitalWrite(this->actLED, LOW);
    char buf[100] = {0x00};
    int start = sprintf(buf, "SEND FRMAE: %04X (%lu) ", f.id, f.length);
    for (int i = 0; i < f.length; i++) {
        start += sprintf(buf+start, "%02X ", f.data.bytes[i]);
    }
    PCCOMM::logToSerial(buf);
    if (!this->can->sendFrame(f)) {
        PCCOMM::logToSerial("Error sending CAN Frame!");
    }
}
 // Attempts to read an avaliable frame from one of the mailboxes
bool canbus_handler::read(CAN_FRAME* f) {
    if (this->can->available() > 0) {
        digitalWrite(this->actLED, LOW);
        this->can->read(*f);
        return true;
    }
    return false;
}

// Locks the interface - Stops another channel from using it
void canbus_handler::lock(uint32_t baud) {
    PCCOMM::logToSerial("Locking CAN Interface");
    if (!this->can) {
        PCCOMM::logToSerial("LOCK - WTF Can is null!?");
        return;
    }
    this->can->init(baud);
    PCCOMM::logToSerial("CAN enabled andbaud set!");
    this->inUse = true;
}

// Unlocks the interface - Marks it as being avaliable for a new channel
void canbus_handler::unlock() {
    PCCOMM::logToSerial("Unlocking CAN Interface");
    if (!this->can) {
        PCCOMM::logToSerial("UNLOCK - WTF Can is null!?");
        return;
    }
    this->can->disable();
    PCCOMM::logToSerial("CAN Disabled!");
    this->inUse = false;
    //TODO Clear Tx and Rx buffers here, and put CAN To sleep
}

// nullptr implies they are not used yet
extern canbus_handler ch0 = canbus_handler(&Can0, DS4); // First avaliable interface  (Use can0)
extern canbus_handler ch1 = canbus_handler(&Can1, DS5); // Second avaliable interface (Use can1)