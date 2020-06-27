#include "pc_comm.h"


namespace PCCOMM {
    PCMSG tempbuf = {0x00};
    uint16_t read_count = 0;
    bool pollMessage(PCMSG *msg) {
        if(SerialUSB.available() > 0) { // Is there enough data in the buffer for
            digitalWrite(DS7_BLUE, LOW);
            uint16_t maxRead = min(SerialUSB.available(), sizeof(PCMSG)-read_count);
            SerialUSB.readBytes((char*)&msg[read_count], maxRead);
            read_count += maxRead;
            if(read_count == sizeof(struct PCMSG)) {
                memcpy(msg, &tempbuf, sizeof(PCMSG));
                read_count = 0;
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
};