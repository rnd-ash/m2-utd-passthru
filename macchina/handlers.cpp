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
    sprintf(buf, "Setting filter. Type: %02X, Mask: %04X, Filter: %04X, Resp: %04X", type, mask, filter, resp, 100);
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
    this->tx_buffer = new uint8_t[1];
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
    this->tx_frame = CAN_FRAME{};
}

bool iso15765_handler::getData() {
    if (this->clearToSend && this->isSending) {
        this->send_buffer(); // We need to check if we can send out packets!
    }
    if (this->can_handle->read(&lastFrame)) {
        char buf2[100];
        int start = sprintf(buf2, "READ FRMAE: %04X (%lu) ", lastFrame.id, lastFrame.length);
        for (int i = 0; i < lastFrame.length; i++) {
            start += sprintf(buf2+start, "%02X ", lastFrame.data.bytes[i]);
        }
        PCCOMM::logToSerial(buf2);
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
                this->send_flow_control();
                return false;
            case 0x20:
                PCCOMM::logToSerial("Multi-Frame body!");
                // At most 7 bytes per frame
                // buflen tells us how many bytes in total for ISO data
                // bufWritePos tells us the current number of bytes written
                bytes_to_copy = min(7, buflen-bufWritePos);
                memcpy(&buf[bufWritePos], &lastFrame.data.bytes[1], bytes_to_copy);
                bufWritePos += bytes_to_copy;
                if (bufWritePos >= buflen && isReceiving) {
                    // Copy complete
                    isReceiving = false;
                    return true;
                }
                rx_count++;
                // Now need to send a new FC frame!
                if (this->rx_count == MAX_BLOCK_SIZE_RX) {
                    this->send_flow_control();
                }
                return false;
            case 0x30:
                // Handle flow control!
                this->tx_sep = lastFrame.data.bytes[2];
                this->tx_bs = lastFrame.data.bytes[3];
                this->clearToSend = true;
                this->tx_last_send_time = millis();
                return false;
            default:
                PCCOMM::logToSerial("Not a valid ISO Frame!");
                return false;
        }
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
        CAN_FRAME f;
        f.extended = false; // TODO Allow for extended frames
        f.id = args[0] << 24 | args[1] << 16 | args[2] << 8 | args[3];
        f.data.byte[0] = len-4;
        f.length = 8; // Always for ISO15765
        f.priority = 4; // Send this frame now!
        f.rtr = 0;
        memcpy(&f.data.bytes[1], &args[4], len-4);
        this->can_handle->transmit(f);
    } else {
        PCCOMM::logToSerial("Sending multiple frames!");
        delete this->tx_buffer;
        this->tx_frame.length = 8; // Always for 15765
        this->tx_frame.id = args[0] << 24 | args[1] << 16 | args[2] << 8 | args[3]; // Set once
        this->tx_frame.priority = 4;
        this->tx_frame.rtr = false;
        this->isSending = true;
        this->clearToSend = false;
        this->tx_buffer = new uint8_t[len-4]; // -4 as the CID now lives in the tx_frame
        this->tx_buffer_size = len-4;
        memcpy(&tx_buffer[0], &args[4], len-4); // Copy the full payload buffer
        this->tx_frame.data.bytes[0] = 0x10;
        this->tx_frame.data.bytes[1] = tx_buffer_size; // Total number of bytes
        memcpy(&tx_frame.data.bytes[2], &tx_buffer[0], 6); // Copy first 6 bytes
        this->tx_buffer_pos = 6;
        this->tx_packet_id = 0x21; // Set the PCI of the next packet
        this->tx_packets_sent = 0;
        this->can_handle->transmit(tx_frame);
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

// Handles sending of 0x2x multi frame payload packets
void iso15765_handler::send_buffer() {
    // Check if too early to send buffer. Min call is used so we don't flood CAN
    if (millis() - tx_last_send_time < min(1, tx_sep)) {
        return;
    }
    PCCOMM::logToSerial("Sending multi frame segment");
    tx_frame.data.bytes[0] = tx_packet_id;
    tx_packet_id++;
    int bytes_left = this->tx_buffer_size - this->tx_buffer_pos;
    memcpy(&tx_frame.data.bytes[1], &this->tx_buffer[tx_buffer_pos], max(7, bytes_left));
    this->can_handle->transmit(tx_frame);
    this->tx_buffer_pos+=7;
    this->tx_packets_sent++;
    if (this->tx_packets_sent == tx_bs) { // Need a clear to send packet again
        PCCOMM::logToSerial("Await new FC Frame");
        this->tx_packets_sent = 0;
        clearToSend = false;
    }
    // Reset PCI if it overflows
    if (this->tx_packet_id == 0x30) {
        this->tx_packet_id = 0x20;
    }
    if (tx_buffer_pos >= this->tx_buffer_size) {
        this->isSending = false;
        this->clearToSend = false;
        PCMSG tx = {0x00};
        tx.cmd_id = CMD_CHANNEL_DATA;
        tx.args[0] = this->channel_id;
        tx.args[1] = ISO15765_SD_INDICATOR;
        tx.arg_size = 2;
        PCCOMM::sendMessage(&tx);
        return;
    }
    tx_last_send_time = millis();
}

void iso15765_handler::send_flow_control() {
    this->rx_count = 0; // Reset
    // Find the correct Response ID in the filters:
    uint32_t tmp_id = getFilterResponseID(lastFrame.id);
    if (tmp_id == 0xFFFFFFFF) {
        PCCOMM::logToSerial("Couldn't find any response ID's!");
        return;
    }
    PCCOMM::logToSerial("Sending Flow control");
    // modify lastFrame so its now the response message
    lastFrame.id = tmp_id;
    // TODO IOCTL based SP and BS
    lastFrame.data.bytes[0] = 0x30; // Tell ECU its clear to send!
    lastFrame.data.bytes[1] = MAX_BLOCK_SIZE_RX; // Block size
    lastFrame.data.bytes[2] = 0x20; // Min seperation time in ms
    lastFrame.data.bytes[3] = 0x00;
    lastFrame.data.bytes[4] = 0x00;
    lastFrame.data.bytes[5] = 0x00;
    lastFrame.data.bytes[6] = 0x00;
    lastFrame.data.bytes[7] = 0x00;
    isReceiving = true;
    this->can_handle->transmit(lastFrame);
}
