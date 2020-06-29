#include "can_handler.h"
#include "pc_comm.h"

canbus_handler::canbus_handler(uint8_t id, uint32_t baud) {
    char buf[30] = {0x00};
    sprintf(buf, "CAN Speed: %lu bps", baud);
    PCCOMM::logToSerial(buf);
    switch(id) {
        case 0:
            PCCOMM::logToSerial("Setting up Can0");
            this->actLED = CAN0_LED;
            this->useCan1 = false;
            PCCOMM::logToSerial("Done");
            this->getIface().begin(baud);
            PCCOMM::logToSerial("Done1");
            break;
        case 1:
            PCCOMM::logToSerial("Setting up Can1");
            this->actLED = CAN1_LED;
            this->useCan1 = true;
            PCCOMM::logToSerial("Done");
            this->getIface().begin(baud);
            PCCOMM::logToSerial("Done1");
            break;
        default:
            PCCOMM::logToSerial("ERROR SETTING UP CAN INVALID ID");
            break;
    }
}

CANRaw canbus_handler::getIface() {
    return this->useCan1 ? Can0 : Can1;
}

void canbus_handler::setFilter(uint32_t canid, uint32_t mask, bool isExtended) {
    this->getIface().setRXFilter(canid, mask, isExtended);
}

void canbus_handler::transmit(CAN_FRAME *f) {
    digitalWrite(this->actLED, LOW);
    char buf[100] = {0x00};
    sprintf(buf, "CAN Sending CAN Frame. ID: 0x%04X - DLC: %d", f->id, f->length);
    PCCOMM::logToSerial(buf);
    if (!this->getIface().sendFrame(*f)) {
        PCCOMM::logToSerial("Error sending CAN Frame!");
    }
}

bool canbus_handler::read(CAN_FRAME *f) {
    if (this->getIface().available() > 0) {
        PCCOMM::logToSerial("CAN Frames avaliable");
        digitalWrite(this->actLED, LOW);
        this->getIface().read(*f);
    }
}

// nullptr implies they are not used yet
extern canbus_handler *h0 = nullptr;
extern canbus_handler *h1 = nullptr;