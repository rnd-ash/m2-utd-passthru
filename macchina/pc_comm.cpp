#include "pc_comm.h"


namespace PCCOMM {
    bool pollMessage(PCMSG *msg) {
        if(SerialUSB.available() >= sizeof(struct PCMSG)) { // Is there enough data in the buffer for
            SerialUSB.readBytes((char*)msg, sizeof(struct PCMSG));
            return true;
        }
        return false;
    }

    void sendMessage(PCMSG *msg) {
        SerialUSB.write((char*)msg, sizeof(struct PCMSG));
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