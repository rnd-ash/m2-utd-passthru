#include "pc_comm.h"
#include "can_handler.h"
#include "channels.h"
#include <map>

#include <M2_12VIO.h>

// -- COMMENT IF YOU HAVE A V3 OR OLDER DEVICE --
#define MACCHINA_V4


M2_12VIO M2IO;
bool connected = false;

// DS2 - Red LED - Status no connection
// DS6 - Green LED - Connected!

#define MAX_CHANNELS 10
channel* channels[MAX_CHANNELS] = {nullptr};
unsigned long active_channels = 0;

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

unsigned long lastPing = 0;
void doPing() {
    comm_msg.cmd_id = CMD_PING;
    comm_msg.arg_size = 8;
    float f = getVoltage();
    memcpy(&comm_msg.args[0], &f, 4);
    comm_msg.args[4] = active_channels;
    PCCOMM::sendMessage(&comm_msg);
}


void create_channel(uint8_t id, uint8_t protocol, unsigned long baud) {
    if (id > MAX_CHANNELS-1) {
        PCCOMM::logToSerial("Channel ID too big - cannot create!"); 
        return;
    }
    if (channels[id] != nullptr) {
        PCCOMM::logToSerial("Cannot create channel. Already in use");
        return;
    }
    channels[id-1] = new channel(id, protocol, baud);
    active_channels++;
}

void destroy_channel(uint8_t id) {
    if (id > MAX_CHANNELS) { 
        PCCOMM::logToSerial("Channel ID too big - cannot destroy!");
        return;
    }
    if (channels[id-1] != nullptr) {
        channels[id-1]->kill_channel();
        delete channels[id];
        channels[id-1] = nullptr;
        active_channels--;
    } else {
        PCCOMM::logToSerial("Cannot destroy channel. Does not exist");
    }
}

void update_channels() { // Tells all channels to check to send data, or to read incomming data
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (channels[i] != nullptr) {
            channels[i]->update();
        }
    }
}

void channel_send_data(uint8_t channelID, uint8_t* data, uint16_t len) {
    PCCOMM::logToSerial("Channel sending data");
    if (channels[channelID-1] != nullptr) {
        PCCOMM::logToSerial("Channel sending data");
        channels[channelID-1]->transmit_data(len, data);
    }  else {
        PCCOMM::logToSerial("Cannot trasmit data on channel. Does not exist");
    }
}

// the loop function runs over and over again until power down or reset
unsigned long l; // Temp buffer;
void loop() {
    if (PCCOMM::pollMessage(&comm_msg)) {
        lastPing = millis();
        connected = true;
        switch (comm_msg.cmd_id) {
            case CMD_PING: // Ping request - Read bat voltage
                doPing();
                break;
            case CMD_EXIT: // User space application quit
                connected = false;
                break;
            case CHANNEL_CREATE: // Create a new channel
                memcpy(&l, &comm_msg.args[2], 4);
                create_channel(comm_msg.args[0], comm_msg.args[1], l);
                break;
            case CHANNEL_DATA: // Send data to a channel
                channel_send_data(comm_msg.args[0], &comm_msg.args[1], comm_msg.arg_size-1);
                break;
            case CHANNEL_DESTROY: // Destroy a channel
                destroy_channel(comm_msg.args[0]);
                break;
            default: // Unknown??
                PCCOMM::logToSerial("Unknown Payload CMD");
                break;
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
    // Turn off Act LEDs (Will turn back on on next message Tx/Rx)
    digitalWrite(DS3, HIGH); // Can 0
    digitalWrite(DS4, HIGH); // Can 1
    digitalWrite(DS5, HIGH); // K-Line
    update_channels();
}
