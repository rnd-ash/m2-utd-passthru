#include "channels.h"
#include "pc_comm.h"

channel::channel(uint8_t id, uint8_t protocol, unsigned long baudRate) {
    this->id = id;
    switch (protocol) {
    case PROTOCOL_CAN:
        this->protocol_handler = new can_handler(baudRate);
        break;
    case PROTOCOL_ISO15765:
        this->protocol_handler = new iso15765_handler(baudRate, this->id);
        break;
    case PROTOCOL_ISO9141:
        this->protocol_handler = new iso9141_handler(baudRate);
        break;
    default:
        break;
    }
}

void channel::transmit_data(uint16_t len, uint8_t* data) {
    this->protocol_handler->transmit(data, len);
}

uint8_t channel::getID() {
    return this->id;
}

void channel::kill_channel() {
    this->protocol_handler->destroy();
    delete protocol_handler;
}

void channel::update() {
    if (this->protocol_handler != nullptr) {
        if (this->protocol_handler->update()) {
            PCMSG tx = {0x00};
            tx.cmd_id = CMD_CHANNEL_DATA;
            tx.arg_size = this->protocol_handler->getBufSize() + 1; // +1 for Channel ID
            tx.args[0] = this->id;
            memcpy(&tx.args[1], this->protocol_handler->getBuf(), this->protocol_handler->getBufSize());
            PCCOMM::sendMessage(&tx);
        }
    }
}

void channel::set_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    if (this->protocol_handler == nullptr) {
        PCCOMM::logToSerial("Cannot set filter - Handler is null");
        return;
    }
    this->protocol_handler->add_filter(id, type, mask, filter, resp);
}

void channel::remove_filter(uint8_t id) {
    if (this->protocol_handler == nullptr) {
        PCCOMM::logToSerial("Cannot remove filter - Handler is null");
        return;
    }
    this->protocol_handler->destroy_filter(id);
}