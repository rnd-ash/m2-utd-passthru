#include "pch.h"
#include "channel.h"
#include "Logger.h"
#include "usbcomm.h"

unsigned long channel_group::addChannel(unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate)
{
    unsigned long chanid = getFreeChannelID();
    if (chanid != 0) {
        channel c = channel(chanid);
        if (!c.setProtocol(ProtocolID)) {
            LOGGER.logError("CHAN_GROUP", "Error setting channel protocol!");
            return 0;
        }
        if (!c.setFlags(Flags)) {
            LOGGER.logError("CHAN_GROUP", "Error setting channel flags!");
            return 0;
        }
        if (!c.setBaud(Baudrate)) {
            LOGGER.logError("CHAN_GROUP", "Error setting channel baudrate!");
            return 0;
        }
        this->channels.emplace(std::make_pair(chanid, c));
        LOGGER.logDebug("CHAN_GROUP", "Created channel OK. Id is %lu", chanid);
    }
    else {
        LOGGER.logError("CHAN_GROUP", "Error creating channel!");
    }
    return chanid;
}

int channel_group::removeChannel(unsigned long channelid)
{
    used[channelid - 1] = false;
    if (channels.find(channelid) != channels.end()) {
        channels.at(channelid).removeChannel();
        channels.erase(channelid);
    }
    return 0;
}

unsigned long channel_group::getFreeChannelID()
{
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (used[i] == false) { // Channel isn't being used
            used[i] = true; // Channel is now marked as used
            return i + 1; // Return +1 to where we are in the array (ID 0 indicates no channel created)
        }
    }
    LOGGER.logError("CHAN_GROUP", "No free channels!");
    return 0;
}

channel* channel_group::getChannelWithID(unsigned long id)
{
    try {
        return &this->channels.at(id);
    }
    catch (std::exception) {
        return nullptr;
    }
}

channel_group channels = channel_group();

channel::channel(unsigned long id)
{
    this->id = id;
    PCMSG m = {
        CHANNEL_CREATE,
        4,
    };
    m.args[0] = this->id;
    usbcomm::sendMessage(&m);
}

bool channel::setProtocol(unsigned long ProtocolID)
{
    switch (ProtocolID) {
    case ISO15765:
        this->handler = new iso15765_handler(this->id);
        return true;
    case ISO9141:
        this->handler = new iso9141_handler(this->id);
        return true;
    case CAN:
        this->handler = new can_handler(this->id);
        return true;
    default:
        LOGGER.logError("CHAN_PROT", "Unsupported protocol %lu", ProtocolID);
        break;
    }
    return false;
}

bool channel::setFlags(unsigned long Flags)
{
    if (this->handler == nullptr) {
        return false;
    }
    this->handler->setFlags(Flags);
    return true;
}

bool channel::setBaud(unsigned long Baudrate)
{
    if (this->handler == nullptr) {
        return false;
    }
    this->handler->setBaud(Baudrate);
    return true;
}

void channel::removeChannel()
{
    PCMSG m = {
        CHANNEL_DESTROY,
        4,
    };
    m.args[0] = this->id;
    usbcomm::sendMessage(&m);
}
