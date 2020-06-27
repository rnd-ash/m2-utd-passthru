#include "pc_comm.h"

#include <M2_12VIO.h>

// -- COMMENT IF YOU HAVE A V3 OR OLDER DEVICE --
#define MACCHINA_V4


M2_12VIO M2IO;
bool connected = false;

// DS2 - Red LED - Status no connection
// DS6 - Green LED - Connected!


PCMSG comm_msg = {0x00};

// the setup function runs once when you press reset or power the board
void setup() {
    SerialUSB.begin(115200);
    pinMode(DS6, OUTPUT);
    pinMode(DS2, OUTPUT);
    pinMode(DS7_GREEN, OUTPUT);
    pinMode(DS7_BLUE, OUTPUT);
    digitalWrite(DS2, LOW); // At startup assume no PC
    digitalWrite(DS6, HIGH);
    digitalWrite(DS7_GREEN, HIGH);
    digitalWrite(DS7_BLUE, HIGH);
    M2IO.Init_12VIO();
}

// https://github.com/kenny-macchina/M2VoltageMonitor/blob/master/M2VoltageMonitor_V4/M2VoltageMonitor_V4.ino
float getVoltage() {
  float voltage=M2IO.Supply_Volts();
  voltage /= 1000;
  voltage=.1795*voltage*voltage-2.2321*voltage+14.596;//calibration curve determined with DSO, assumed good
  //additional correction for M2 V4
#ifdef MACCHINA_V4
  voltage=-.0168*voltage*voltage+1.003*voltage+1.3199;//calibration curve determined with DMM, assumed good (M2 V4 only!)
#endif
  return voltage;
}

void doPing() {
    comm_msg.cmd_id = CMD_PING;
    comm_msg.arg_size = 4;
    float f = getVoltage();
    memcpy(&comm_msg.args[0], &f, 4);
    PCCOMM::sendMessage(&comm_msg);
}

unsigned long lastPing = 0;

// the loop function runs over and over again until power down or reset
void loop() {
    if (PCCOMM::pollMessage(&comm_msg)) {
        lastPing = millis();
        connected = true;
        if (comm_msg.cmd_id == CMD_PING) {
            doPing();
        }
        if (comm_msg.cmd_id == CMD_EXIT) {
            connected = false;
        }
    }
    
    // Do status LED thing to show connected or not
    if(millis() - lastPing > 5000 ||!connected) { // Not connected
        digitalWrite(DS6, HIGH);
        digitalWrite(DS2, LOW);
    } else { // Connected
        digitalWrite(DS6, LOW);
        digitalWrite(DS2, HIGH);
    }
}
