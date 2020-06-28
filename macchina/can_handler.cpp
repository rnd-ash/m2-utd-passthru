#include "can_handler.h"
#include "pc_comm.h"

canbus_handler::canbus_handler(uint8_t id, uint32_t baud) {
    switch(id) {
        case 0:
         PCCOMM::logToSerial("Setting up Can0");
            this->actLED = CAN0_LED;
            this->can = &Can0;
            this->can->begin(baud);
            PCCOMM::logToSerial("Done");
            break;
        case 1:
            PCCOMM::logToSerial("Setting up Can1");
            this->actLED = CAN1_LED;
            this->can = &Can1;
            this->can->begin(baud);
            PCCOMM::logToSerial("Done");
            break;
        default:
            break;
    }
}

void canbus_handler::setFilter(uint32_t canid, uint32_t mask, bool isExtended) {
    this->can->setRXFilter(canid, mask, isExtended);
}

void canbus_handler::transmit(CAN_FRAME *f) {
    digitalWrite(this->actLED, LOW);
    this->can->sendFrame(*f);
    digitalWrite(this->actLED, HIGH);
}

bool canbus_handler::read(CAN_FRAME *f) {
    if (this->can->available() > 0) {
        digitalWrite(this->actLED, LOW);
        this->can->read(*f);
        digitalWrite(this->actLED, HIGH);
    }
}

extern canbus_handler *h0 = nullptr;
extern canbus_handler *h1 = nullptr;