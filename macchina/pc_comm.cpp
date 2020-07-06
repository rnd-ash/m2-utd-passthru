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

#include "pc_comm.h"
#include "j2534_mini.h"

namespace PCCOMM {
    char tempbuf[520] = {0x00};
    uint16_t read_count = 0;
    uint8_t lastID = 0x00;
    bool pollMessage(PCMSG *msg) {
        if(SerialUSB.available() > 0) { // Is there enough data in the buffer for

            // Calculate how many bytes to read (min of avaliable bytes, or left to read to complete the data)
            uint16_t maxRead = min(SerialUSB.available(), sizeof(PCMSG)-read_count);
            digitalWrite(DS7_RED, LOW);
            SerialUSB.readBytes(&tempbuf[read_count], maxRead);
            digitalWrite(DS7_RED, HIGH);
            read_count += maxRead;

            // Size OK now, full payload received
            if(read_count == sizeof(struct PCMSG)) {
                memcpy(msg, &tempbuf, sizeof(PCMSG));
                read_count = 0;
                memset(tempbuf, 0x00, sizeof(tempbuf)); // Reset buffer
                lastID = msg->msg_id; // Set this for response
                return true;
            }
        }
        return false;
    }

    void sendMessage(PCMSG *msg) {
        digitalWrite(DS7_GREEN, LOW);
        SerialUSB.write((char*)msg, sizeof(struct PCMSG));
        digitalWrite(DS7_GREEN, HIGH);
    }

    void logToSerial(char* msg) {
        uint16_t len = max(strlen(msg), 512);
        PCMSG res = {0x00};
        res.cmd_id = CMD_LOG;
        res.arg_size = len;
        memcpy(res.args, msg, len);
        sendMessage(&res);
    }

    void respondOK(uint8_t cmd_id, uint8_t* resp_data, uint16_t resp_data_len) {
        PCMSG send = {
            cmd_id | CMD_RES_FROM_CMD,
            STATUS_NOERROR,
            resp_data_len+1
        };
        send.msg_id = lastID;
        memcpy(&send.args[1], &resp_data, resp_data_len);
        sendMessage(&send);
    }

    void respondFail(uint8_t cmd_id, uint8_t err_code, char* msg) {
        int len = strlen(msg);
        PCMSG send = {
            cmd_id | CMD_RES_FROM_CMD,
            err_code,
            len
        };
        send.msg_id = lastID;
        memcpy(&send.args, &msg, len);
        sendMessage(&send);
    }
};