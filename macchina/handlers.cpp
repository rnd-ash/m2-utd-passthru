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

#include "channels.h"
#include "pc_comm.h"


handler::handler(unsigned long baud) {
    this->buflen = 0;
}

uint32_t handler::getFilterResponseID(uint32_t rxID) {
    for (int i = 0; i < MAX_FILTERS_PER_HANDLER; i++) {
        if (filters[i]->mask & rxID == filters[i]->filter) {
            return filters[i]->flow;
        }
    }
    return 0xFFFFFFFF; // Invalid CID
}

void handler::add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    if (id > MAX_FILTERS_PER_HANDLER-1) {
        PCCOMM::logToSerial("Cannot add filter - ID is out of range");
        return;
    }
    if (this->filters[id-1] != nullptr) {
        PCCOMM::logToSerial("Cannot add filter - Already in use");
        return;
    }
    filters[id-1] = new handler_filter {
        id,
        type,
        mask,
        filter,
        resp
    };
    char buf[100] = {0x00};
    sprintf(buf, "Setting filter. Type: %02X, Mask: %04X, Filter: %04X, Resp: %04X", type, mask, filter, resp);
    PCCOMM::logToSerial(buf);
}

bool handler::update() {
    return this->getData();
}

uint8_t* handler::getBuf() {
    return this->buf;
}

uint8_t handler::getBufSize() {
    return this->buflen;
}

void handler::destroy_filter(uint8_t id) {
    if (this->filters[id-1] != nullptr) {
        delete filters[id-1];
        filters[id-1] = nullptr;
    } else {
         PCCOMM::logToSerial("Cannot remove filter - doesn't exist");
    }
}

void handler::destroy() {
    delete this->buf;
}

// CAN stuff (Normal CAN Payloads)

can_handler::can_handler(unsigned long baud) : handler(baud) {
    PCCOMM::logToSerial("Setting up CAN Handler");
    this->buf = new uint8_t[12]; // Max (4 bytes for ID, 8 for DLC)
    if (ch0.isFree()) {
        ch0.lock(baud);
        this->can_handle = &ch0;
    } else if (ch1.isFree()) {
        ch1.lock(baud);
        this->can_handle = &ch1;
    } else {
        PCCOMM::logToSerial("NO AVALIABLE CAN HANDLERS!");
        return;
    }
    this->lastFrame = CAN_FRAME{};
}

bool can_handler::getData() {
    return false;
}

void can_handler::destroy() {
    this->can_handle->unlock();
}

void can_handler::transmit(uint8_t* args, uint16_t len) {

}

void can_handler::add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    handler::add_filter(id, type, mask, filter, resp);
    if (type == PROTOCOL_FILTER_BLOCK) { // Block filter, so allow everything, then we do bitwising in SW
        this->can_handle->setFilter(0x7FF, 0x00, false);
    } else { // Pass filter, so allow into mailboxes
        this->can_handle->setFilter(mask, filter & 0x7FF, false); // TODO Do Extended filtering
    }
}

// ISO 9141 stuff (K-Line)

iso9141_handler::iso9141_handler(unsigned long baud) : handler(baud) {
    PCCOMM::logToSerial("Setting up ISO9141 Handler");
}

bool iso9141_handler::getData() {
    return false;
}

void iso9141_handler::destroy() {

}

void iso9141_handler::transmit(uint8_t* args, uint16_t len) {
    // TODO Kline stuff
}

void iso9141_handler::add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    handler::add_filter(id, type, mask, filter, resp);
}


// ISO 15765 stuff (Big CAN Payloads)

iso15765_handler::iso15765_handler(unsigned long baud, uint8_t id) : handler(baud) {
    this->buf = new uint8_t[1]; // Temporary (Will get destroyed as soon as we read a message)
    this->channel_id = id;
    PCCOMM::logToSerial("Setting up ISO15765 Handler");
    if (ch0.isFree()) {
        ch0.lock(baud);
        this->can_handle = &ch0;
    } else if (ch1.isFree()) {
        ch1.lock(baud);
        this->can_handle = &ch1;
    } else {
        PCCOMM::logToSerial("NO AVALIABLE CAN HANDLERS!");
        return;
    }
    this->lastFrame = CAN_FRAME{};
}

bool iso15765_handler::getData() {
    if (this->can_handle->read(&lastFrame)) {
        char buf2[100];
        int start = sprintf(buf2, "READ FRMAE: %04X (%lu) ", lastFrame.id, lastFrame.length);
        for (int i = 0; i < lastFrame.length; i++) {
            start += sprintf(buf2+start, "%02X ", lastFrame.data.bytes[i]);
        }
        PCCOMM::logToSerial(buf2);
        uint32_t tmp_id = 0xFFFFFFFF; // Only used for FC messages
        uint8_t bytes_to_copy = 0; // Used for multi-frame Rx
        switch(lastFrame.data.bytes[0] & 0xF0) {
            case 0x00:
                delete buf; // Wipe the old buffer (If any)
                buf = new uint8_t[lastFrame.data.bytes[0] + 4]; // Data size is ID (4) + ISO Size
                this->buflen = lastFrame.data.bytes[0] + 4;
                // Copy ID
                buf[0] = lastFrame.id >> 24;
                buf[1] = lastFrame.id >> 16;
                buf[2] = lastFrame.id >> 8;
                buf[3] = lastFrame.id;
                // Copy all the bytes in the payload section of the frame
                memcpy(&buf[4], &lastFrame.data.bytes[1], lastFrame.data.bytes[0]);
                return true;
            case 0x10:
                PCCOMM::logToSerial("Multi-Frame head!");
                // First frame indication Tx back to driver
                this->sendFF(lastFrame.id);
                // Set buffer to real size
                delete buf;
                buf = new uint8_t[lastFrame.data.bytes[1] + 4];
                this->bufWritePos = 10; // When finished, we will be at the 10th byte
                this->buflen = lastFrame.data.bytes[1] + 4; // Set the full buffer size
                buf[0] = lastFrame.id >> 24;
                buf[1] = lastFrame.id >> 16;
                buf[2] = lastFrame.id >> 8;
                buf[3] = lastFrame.id;
                // Copy all the bytes in the payload section of the frame
                memcpy(&buf[4], &lastFrame.data.bytes[2], 6); // Copy the first 6 bytes
                // Need to now send the FC frame
                
                // Find the correct Response ID in the filters:
                tmp_id = getFilterResponseID(lastFrame.id);
                if (tmp_id == 0xFFFFFFFF) {
                    PCCOMM::logToSerial("Couldn't find any response ID's!");
                    return false;
                }
                // modify lastFrame so its now the response message
                lastFrame.id = tmp_id;
                // TODO IOCTL based SP and BS
                lastFrame.data.bytes[0] = 0x30; // Tell ECU its clear to send!
                lastFrame.data.bytes[1] = 0x08; // Block size
                lastFrame.data.bytes[2] = 0x20; // Min seperation time in ms
                lastFrame.data.bytes[3] = 0x00;
                lastFrame.data.bytes[4] = 0x00;
                lastFrame.data.bytes[5] = 0x00;
                lastFrame.data.bytes[6] = 0x00;
                lastFrame.data.bytes[7] = 0x00;
                this->can_handle->transmit(lastFrame);
                return false;
            case 0x20:
                PCCOMM::logToSerial("Multi-Frame body!");
                // At most 7 bytes per frame
                // buflen tells us how many bytes in total for ISO data
                // bufWritePos tells us the current number of bytes written
                bytes_to_copy = min(7, buflen-bufWritePos);
                memcpy(&buf[bufWritePos], &lastFrame.data.bytes[1], bytes_to_copy);
                bufWritePos += bytes_to_copy;
                if (bufWritePos >= bytes_to_copy) {
                    // Copy complete
                    return true;
                }
                return false;
            case 0x30:
                PCCOMM::logToSerial("Cannot handle flow control Frame!");
                return false;
            default:
                PCCOMM::logToSerial("Not a valid ISO Frame!");
                return false;
        }
        return true;
    }
    return false;
}

void iso15765_handler::destroy() {
    this->can_handle->unlock();
}

void iso15765_handler::transmit(uint8_t* args, uint16_t len) {
    if (this->can_handle == nullptr) {
        PCCOMM::logToSerial("ISO15765 cannot transmit - Handler is null");
        return;
    }
    if (len-4 < 7) {
        CAN_FRAME f = {0x00};
        f.extended = false; // TODO Allow for extended frames
        f.id = args[0] << 24 | args[1] << 16 | args[2] << 8 | args[3];
        f.data.byte[0] = len-4;
        f.length = 8; // Always for ISO15765
        f.priority = 4; // Send this frame now!
        f.rtr = 0;
        memcpy(&f.data.bytes[1], &args[4], len-4);
        this->can_handle->transmit(f);
    } else {
        PCCOMM::logToSerial("TODO ISO Multi frame");
        // TODO Multi-frame packets
    }
}

void iso15765_handler::add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    handler::add_filter(id, type, mask, filter, resp);
    if (type == PROTOCOL_FILTER_BLOCK) { // Block filter, so allow everything, then we do bitwising in SW
        this->can_handle->setFilter(0x0, 0x0, false);
    } else { // Pass filter, so allow into mailboxes
        this->can_handle->setFilter(filter & 0x7FF, mask, false); // TODO Do Extended filtering
    }
}

void iso15765_handler::sendFF(uint32_t canid) {
    PCMSG tx = {0x00};
    tx.cmd_id = CMD_CHANNEL_DATA;
    tx.args[0] = this->channel_id;
    tx.args[1] = ISO15765_FF_INDICATOR; // As this is never true in a CAN Frame, it can be used as a flag
    tx.args[2] = canid >> 24;
    tx.args[3] = canid >> 16;
    tx.args[4] = canid >> 8;
    tx.args[5] = canid;
    tx.arg_size = 6;
    PCCOMM::sendMessage(&tx);
}
