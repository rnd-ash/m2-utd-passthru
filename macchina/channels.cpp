#include "channels.h"


channel::channel(uint8_t id) {
    this->id = id;
}

uint8_t channel::getID() {
    return this->id;
}

void channel::kill_channel() {
    // TODO - Kill channel
}

void channel::update() {
    // TODO - update channel
}