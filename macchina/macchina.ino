#include "pc_comm.h"

// DS2 - Red LED - Status no connection
// DS6 - Green LED - Connected!


PCMSG comm_msg = {0x00};

// the setup function runs once when you press reset or power the board
void setup() {
    SerialUSB.begin(0);
    pinMode(DS6, OUTPUT);
    pinMode(DS2, OUTPUT);
    digitalWrite(DS2, LOW); // At startup assume no PC
}

// the loop function runs over and over again until power down or reset
void loop() {
    if (PCCOMM::pollMessage(&comm_msg)) {
        char buf[100] = {0x00};
        sprintf(buf, "Received msg. CMD: %04X, size: %d", comm_msg.cmd_id, comm_msg.arg_size);
        PCCOMM::logToSerial(buf);
    }
}
