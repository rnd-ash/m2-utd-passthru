#include "pc_comm.h"
#include "can_handler.h"
#include "channels.h"
#include "j2534_mini.h"
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
    pinMode(DS6, OUTPUT); // Green
    pinMode(DS5, OUTPUT); // Yellow
    pinMode(DS4, OUTPUT); // Yellow
    pinMode(DS3, OUTPUT); // Yellow
    pinMode(DS2, OUTPUT); // Red
    pinMode(DS7_GREEN, OUTPUT); // RGB (Green)
    pinMode(DS7_BLUE, OUTPUT);  // RGB (Blue)
    pinMode(DS7_RED, OUTPUT);   // RGB (Red)
    digitalWrite(DS2, LOW); // At startup assume no PC
    digitalWrite(DS6, HIGH);
    digitalWrite(DS7_GREEN, HIGH);
    digitalWrite(DS7_BLUE, HIGH);
    digitalWrite(DS7_RED, HIGH);
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
    char buf[50];
    sprintf(buf, "Voltage: %.2f, Active channels: %lu", getVoltage(), active_channels);
    PCCOMM::logToSerial(buf);
}


void create_channel(uint8_t id, uint8_t protocol, unsigned long baud) {
    if (id > MAX_CHANNELS-1) {
       PCCOMM::respondFail(CMD_CHANNEL_CREATE, ERR_INVALID_CHANNEL_ID, "Channel ID is too large");
        return;
    }
    if (channels[id] != nullptr) {
        PCCOMM::respondFail(CMD_CHANNEL_CREATE, ERR_CHANNEL_IN_USE, "Channel ID is already in use");
        return;
    }
    channels[id-1] = new channel(id, protocol, baud);
    active_channels++;
    uint8_t res[1] = {0x00};
    PCCOMM::respondOK(CMD_CHANNEL_CREATE, res, 1);
}

void destroy_channel(uint8_t id) {
    if (id > MAX_CHANNELS) { 
        PCCOMM::respondFail(CMD_CHANNEL_DESTROY, ERR_INVALID_CHANNEL_ID, "Channel ID is too large");
        return;
    }
    if (channels[id-1] != nullptr) {
        channels[id-1]->kill_channel();
        delete channels[id];
        channels[id-1] = nullptr;
        active_channels--;
        uint8_t res[1] = {0x00};
        PCCOMM::respondOK(CMD_CHANNEL_DESTROY, res, 1);
    } else {
        PCCOMM::respondFail(CMD_CHANNEL_DESTROY, ERR_INVALID_CHANNEL_ID, "Channel ID already exists");
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
    if (channels[channelID-1] != nullptr) {
        channels[channelID-1]->transmit_data(len, data);
    }  else {
        PCCOMM::logToSerial("Cannot trasmit data on channel. Does not exist");
    }
}

void channel_set_filter(uint8_t channelID, uint8_t* args) {
    if (channels[channelID-1] != nullptr) {
        uint8_t id = args[0];
        uint8_t type = args[1];
        // Most horrible C++ code award goes here
        // Forcing bit shift rather than memcpy to avoid bytes being swapped around
        uint32_t mask = args[2] << 24 | args[3] << 16 | args[4] << 8 | args[5];
        uint32_t filter = args[6] << 24 | args[7] << 16 | args[8] << 8 | args[9];
        uint32_t resp = args[10] << 24 | args[11] << 16 | args[12] << 8 | args[13];
        channels[channelID-1]->set_filter(id, type, mask, filter, resp);
    }  else {
        PCCOMM::logToSerial("Cannot set channel filter. Does not exist");
    }
}

void channel_remove_filter(uint8_t channelID, uint8_t id) {
    if (channels[channelID-1] != nullptr) {
        channels[channelID-1]->remove_filter(id);
    }  else {
        PCCOMM::logToSerial("Cannot remove channel filter. Does not exist");
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
            case CMD_CHANNEL_CREATE: // Create a new channel
                memcpy(&l, &comm_msg.args[2], 4);
                create_channel(comm_msg.args[0], comm_msg.args[1], l);
                break;
            case CMD_CHANNEL_DATA: // Send data to a channel
                channel_send_data(comm_msg.args[0], &comm_msg.args[1], comm_msg.arg_size-1);
                break;
            case CMD_CHANNEL_SET_FILTER:
                channel_set_filter(comm_msg.args[0], &comm_msg.args[1]);
                break;
            case CMD_CHANNEL_REM_FILTER:
                channel_remove_filter(comm_msg.args[0], comm_msg.args[1]);
                break;
            case CMD_CHANNEL_DESTROY: // Destroy a channel
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
