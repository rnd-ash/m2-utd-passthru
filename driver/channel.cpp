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
        // Now channel is setup here, deploy on the Macchina!
        if (!c.setMacchinaChannel()) {
            LOGGER.logError("CHAN_GROUP", "Error deploying channel on macchina!");
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

int channel_group::send_payload(unsigned long channel_id, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long timeout)
{
    channel* chan = getChannelWithID(channel_id);
    if (chan == nullptr) {
        return ERR_INVALID_CHANNEL_ID;
    }
    LOGGER.logInfo("CHAN_SEND", "Sending %lu messages to channel %lu", *pNumMsgs, channel_id);
    for (unsigned long i = 0; i < *pNumMsgs; i++) {
        chan->sendPayload(&pMsg[i]);
    }
    return STATUS_NOERROR;
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
    this->macchinaProtocolID = 0x00;
}

bool channel::setProtocol(unsigned long ProtocolID)
{

    switch (ProtocolID) {
    case ISO15765:
        this->handler = new iso15765_handler(this->id);
        this->macchinaProtocolID = PROTOCOL_ISO15765;
        break;
    case ISO9141:
        this->handler = new iso9141_handler(this->id);
        this->macchinaProtocolID = PROTOCOL_ISO9141;
        break;
    case CAN:
        this->handler = new can_handler(this->id);
        this->macchinaProtocolID = PROTOCOL_CAN;
        break;
    default:
        LOGGER.logError("CHAN_PROT", "Unsupported protocol %lu", ProtocolID);
        return false;
    }
    return true;
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

bool channel::setMacchinaChannel()
{
    // Paylaod args format
    // 0 - Channel ID
    // 1 - Protocol ID
    // 2-6 - Baud rate of channel
    PCMSG m = {
        CMD_CHANNEL_CREATE,
        6,
        (uint8_t)this->id,
        this->macchinaProtocolID
    };
    m.arg_size = 6;
    unsigned long baud = handler->getBaud();
    memcpy(&m.args[2], &baud, 4);
    return usbcomm::sendMessage(&m);
}

int channel::sendPayload(PASSTHRU_MSG* msg)
{
    // Cannot send enough data
    if (msg->DataSize > 508) {
        return ERR_BUFFER_FULL;
    }
    PCMSG m = { 0x00 };
    m.arg_size = msg->DataSize + 1; // +1 for channel ID
    m.cmd_id = CMD_CHANNEL_DATA; // Sending data
    m.args[0] = (uint8_t)this->id;
    memcpy(&m.args[1], msg->Data, msg->DataSize);
    usbcomm::sendMessage(&m);
    return 0;
}

void channel::removeChannel()
{
    PCMSG m = {
        CMD_CHANNEL_DESTROY,
        1,
    };
    m.args[0] = this->id;
    usbcomm::sendMessage(&m);
}
