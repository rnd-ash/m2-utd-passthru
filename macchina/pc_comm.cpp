#include "pc_comm.h"


namespace PCCOMM {
    char tempbuf[512] = {0x00};
    uint16_t read_count = 0;

    bool pollMessage(PCMSG *msg) {
        if(SerialUSB.available() > 0) { // Is there enough data in the buffer for
            digitalWrite(DS7_BLUE, LOW);

            // Calculate how many bytes to read (min of avaliable bytes, or left to read to complete the data)
            uint16_t maxRead = min(SerialUSB.available(), sizeof(PCMSG)-read_count);
            //uint16_t readBytes = 0;
            //while (countRead < maxRead) {
            //    tempbuf[read_count + readBytes] = (char)SerialUSB.read();
            //}

            SerialUSB.readBytes(&tempbuf[read_count], maxRead);
            read_count += maxRead;

            // Size OK now, full payload received
            if(read_count == sizeof(struct PCMSG)) {
                memcpy(msg, &tempbuf, sizeof(PCMSG));
                read_count = 0;
                memset(tempbuf, 0x00, sizeof(tempbuf)); // Reset buffer
                digitalWrite(DS7_BLUE, HIGH);
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
        uint16_t len = max(strlen(msg), 508);
        PCMSG res = {
            CMD_LOG,
            len
        };
        memcpy(res.args, msg, len);
        sendMessage(&res);
    }

    void respondOK(uint8_t cmd_id, uint8_t* resp_data, uint16_t resp_data_len) {
        PCMSG send = {
            cmd_id | CMD_RES_FROM_CMD,
            resp_data_len+1,
            CMD_RES_STATE_OK
        };
        memcpy(&send.args[1], &resp_data, resp_data_len);
        sendMessage(&send);
    }

    void respondFail(uint8_t cmd_id, uint8_t err_code, char* msg) {
        int len = strlen(msg);
        PCMSG send = {
            cmd_id | CMD_RES_FROM_CMD,
            len+2,
            CMD_RES_STATE_FAIL,
            err_code
        };
        memcpy(&send.args[2], &msg, len);
        sendMessage(&send);
    }
};