#include "can_handler.h"


canbus_handler::canbus_handler(uint8_t id, uint32_t baud) {
    switch(id) {
        case 0:
            this->actLED = CAN0_LED;
            this->can = &Can0;
            this->can->set_baudrate(baud);
            break;
        case 1:
            this->actLED = CAN1_LED;
            this->can = &Can1;
            this->can->set_baudrate(baud);
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