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

#ifndef PC_COMM_H
#define PC_COMM_H

#include <stdint.h>
#include <Arduino.h>

struct PCMSG { // Total 512 bytes
    uint8_t cmd_id;
    uint8_t resp_code; // J2534 response code
    uint16_t arg_size;
    uint8_t args[512];
    uint8_t msg_id;
    bool __require_response;
};


namespace PCCOMM {
    bool pollMessage(PCMSG *msg);
    void sendMessage(PCMSG *msg);
    void logToSerial(char* msg);
    void respondOK(uint8_t cmd_id, uint8_t* resp_data, uint16_t resp_data_len);
    void respondFail(uint8_t cmd_id, uint8_t err_code, char* msg);
};


// Command ID's for Misc
#define CMD_LOG  0x01
#define CMD_PING 0x02

// Channel command ID's
#define CMD_CHANNEL_CREATE     0x03 // Creating a new channel
#define CMD_CHANNEL_DATA       0x04 // Send data to/from channel
#define CMD_CHANNEL_DESTROY    0x05 // Killing a channel
#define CMD_CHANNEL_IOCTL_REQ  0x06 // IOCTL request to device
#define CMD_CHANNEL_IOCTL_RESP 0x07 // IOCTL Response from device
#define CMD_CHANNEL_SET_FILTER 0x08 // Add a filter to a channel
#define CMD_CHANNEL_REM_FILTER 0x09 // Remove a filter from a channel;

// Command responses (From macchina)
#define CMD_RES_FROM_CMD       0xA0 // This gets put onto the first nibble of a CMD Id if its the Macchina responding from it 
#define CMD_RES_STATE_OK       0x10 // Command sent to Macchina was OK
#define CMD_RES_STATE_FAIL     0x20 // Command send to Macchina failed, args contains error message

#define CMD_EXIT 0xFF // If sent, device will reset itself back into default state (Assume use app has quit)


#endif