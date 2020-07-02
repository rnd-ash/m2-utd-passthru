#include "channels.h"
#include "pc_comm.h"


handler::handler(unsigned long baud) {
    this->buflen = 0;
}

void handler::add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    if (id > MAX_FILTERS_PER_HANDLER-1) {
        PCCOMM::logToSerial("Cannot add filter - ID is out of range");
        return;
    }
    if (this->filters[id-1] != nullptr) {
        PCCOMM::logToSerial("Cannot add filter - Already in use");
        return;
    }
    filters[id-1] = new handler_filter {
        id,
        type,
        mask,
        filter,
        resp
    };
    char buf[100] = {0x00};
    sprintf(buf, "Setting filter. Type: %02X, Mask: %04X, Filter: %04X, Resp: %04X", type, mask, filter, resp);
    PCCOMM::logToSerial(buf);
}

bool handler::update() {
    return this->getData();
}

uint8_t* handler::getBuf() {
    return this->buf;
}

uint8_t handler::getBufSize() {
    return this->buflen;
}

void handler::destroy_filter(uint8_t id) {
    if (this->filters[id-1] != nullptr) {
        delete filters[id-1];
        filters[id-1] = nullptr;
    } else {
         PCCOMM::logToSerial("Cannot remove filter - doesn't exist");
    }
}

void handler::destroy() {
    delete this->buf;
}

// CAN stuff (Normal CAN Payloads)

can_handler::can_handler(unsigned long baud) : handler(baud) {
    PCCOMM::logToSerial("Setting up CAN Handler");
    this->buf = new uint8_t[12]; // Max (4 bytes for ID, 8 for DLC)
    if (ch0.isFree()) {
        ch0.lock(baud);
        this->can_handle = &ch0;
    } else if (ch1.isFree()) {
        ch1.lock(baud);
        this->can_handle = &ch1;
    } else {
        PCCOMM::logToSerial("NO AVALIABLE CAN HANDLERS!");
        return;
    }
    this->lastFrame = CAN_FRAME{};
}

bool can_handler::getData() {
    return false;
}

void can_handler::destroy() {
    this->can_handle->unlock();
}

void can_handler::transmit(uint8_t* args, uint16_t len) {

}

void can_handler::add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    handler::add_filter(id, type, mask, filter, resp);
    if (type == PROTOCOL_FILTER_BLOCK) { // Block filter, so allow everything, then we do bitwising in SW
        this->can_handle->setFilter(0x7FF, 0x00, false);
    } else { // Pass filter, so allow into mailboxes
        this->can_handle->setFilter(mask, filter & 0x7FF, false); // TODO Do Extended filtering
    }
}

// ISO 9141 stuff (K-Line)

iso9141_handler::iso9141_handler(unsigned long baud) : handler(baud) {
    PCCOMM::logToSerial("Setting up ISO9141 Handler");
}

bool iso9141_handler::getData() {
    return false;
}

void iso9141_handler::destroy() {

}

void iso9141_handler::transmit(uint8_t* args, uint16_t len) {
    // TODO Kline stuff
}

void iso9141_handler::add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    handler::add_filter(id, type, mask, filter, resp);
}


// ISO 15765 stuff (Big CAN Payloads)

iso15765_handler::iso15765_handler(unsigned long baud) : handler(baud) {
    this->buf = new uint8_t[12]; // Max (4 bytes for ID, 8 for DLC)
    PCCOMM::logToSerial("Setting up ISO15765 Handler");
    if (ch0.isFree()) {
        ch0.lock(baud);
        this->can_handle = &ch0;
    } else if (ch1.isFree()) {
        ch1.lock(baud);
        this->can_handle = &ch1;
    } else {
        PCCOMM::logToSerial("NO AVALIABLE CAN HANDLERS!");
        return;
    }
    this->lastFrame = CAN_FRAME{};
}

bool iso15765_handler::getData() {
    if (this->can_handle->read(lastFrame)) {
        memcpy(&this->buf[0], &lastFrame.id, 4);
        memcpy(&this->buf[4], lastFrame.data.bytes, lastFrame.length);
        this->buflen = 4+lastFrame.length; 
        return true;
    }
    return false;
}

void iso15765_handler::destroy() {
    this->can_handle->unlock();
}

void iso15765_handler::transmit(uint8_t* args, uint16_t len) {
    if (this->can_handle == nullptr) {
        PCCOMM::logToSerial("ISO15765 cannot transmit - Handler is null");
        return;
    }
    if (len-4 < 7) {
        CAN_FRAME f = {0x00};
        f.extended = false; // TODO Allow for extended frames
        f.id = args[0] << 24 | args[1] << 16 | args[2] << 8 | args[3];
        f.data.byte[0] = len-4;
        f.length = 8; // Always for ISO15765
        f.priority = 4; // Send this frame now!
        f.rtr = 0;
        memcpy(&f.data.bytes[1], &args[4], len-4);
        this->can_handle->transmit(f);
    } else {
        // TODO Multi-frame packets
    }
}

void iso15765_handler::add_filter(uint8_t id, uint8_t type, uint32_t mask, uint32_t filter, uint32_t resp) {
    handler::add_filter(id, type, mask, filter, resp);
    this->can_handle->setFilter(0x0, 0x0, false);
    /*
    if (type == PROTOCOL_FILTER_BLOCK) { // Block filter, so allow everything, then we do bitwising in SW
        this->can_handle->setFilter(0x0, 0x0, false);
    } else { // Pass filter, so allow into mailboxes
        this->can_handle->setFilter(filter & 0x7FF, mask, false); // TODO Do Extended filtering
    }
    */
}