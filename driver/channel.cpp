#include "pch.h"
#include "channel.h"
#include "Logger.h"
#include "globals.h"



std::tuple<int, unsigned long> channel_group::addChannel(unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate)
{
    unsigned long chanid = getFreeChannelID();
    if (chanid != 0) {
        channel c = channel(chanid);
        int res;
        // Firstly, set protocol
        res = c.setProtocol(ProtocolID);
        if (res != STATUS_NOERROR) {
            LOGGER.logError("CHAN_GROUP", "Error setting channel protocol!");
            return std::make_tuple(res, 0);
        }
        // Then, set channel flags
        res = c.setFlags(Flags);
        if (res != STATUS_NOERROR) {
            LOGGER.logError("CHAN_GROUP", "Error setting channel flags!");
            return std::make_tuple(res, 0);
        }
        // Set channel baud rate
        res = c.setBaud(Baudrate);
        if (res != STATUS_NOERROR) {
            LOGGER.logError("CHAN_GROUP", "Error setting channel baudrate!");
            return std::make_tuple(res, 0);
        }
        // Now channel is setup here, deploy on the Macchina!
        res = c.setMacchinaChannel();
        if (res != STATUS_NOERROR) {
            LOGGER.logError("CHAN_GROUP", "Error deploying channel on macchina!");
            return std::make_tuple(res, 0);
        }
        this->channels.emplace(std::make_pair(chanid, c));
        LOGGER.logDebug("CHAN_GROUP", "Created channel OK. Id is %lu", chanid);
    }
    else {
        LOGGER.logError("CHAN_GROUP", "Error creating channel!");
        globals::setErrorString("No more free channels");
        return std::make_tuple(ERR_FAILED, chanid);
    }
    return std::make_tuple(STATUS_NOERROR, chanid);
}

int channel_group::removeChannel(unsigned long channelid)
{
    used[channelid - 1] = false;
    if (channels.find(channelid) != channels.end()) {
        int ret = channels.at(channelid).removeChannel();
        channels.erase(channelid);
        return ret;
    }
    return ERR_INVALID_CHANNEL_ID; // Channel doesn't exist??
}

void channel_group::recvPayload(PCMSG* m)
{
    // We know its channel data coming into this function
    channel* chan = getChannelWithID(m->args[0]);
    if (chan == nullptr) {
        LOGGER.logError("CHAN_RECV", "Cannot send data to requested channel %d (Channel does not exist)", m->args[0]);
    }
    else {
        chan->recvData(&m->args[1], m->arg_size-1);
    }
}

int channel_group::requestChannelData(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
   channel* chan = getChannelWithID(ChannelID);
   if (chan == nullptr) {
       return ERR_INVALID_CHANNEL_ID;
   }
   return chan->requestData(pMsg, pNumMsgs, Timeout);
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

int channel_group::setFilter(unsigned long channel_id, unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID)
{
    channel* chan = getChannelWithID(channel_id);
    if (chan == nullptr) {
        return ERR_INVALID_CHANNEL_ID;
    }  
    return chan->setFilter(FilterType, pMaskMsg, pPatternMsg, pFlowControlMsg, pFilterID);
}

int channel_group::remove_filter(unsigned long channel_id, unsigned long filterID)
{
    channel* chan = getChannelWithID(channel_id);
    if (chan == nullptr) {
        return ERR_INVALID_CHANNEL_ID;
    }
    return chan->remove_filter(filterID);
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

int channel::setProtocol(unsigned long ProtocolID)
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
        return ERR_INVALID_PROTOCOL_ID;
    }
    return STATUS_NOERROR;
}

int channel::setFlags(unsigned long Flags)
{
    if (this->handler == nullptr) {
        globals::setErrorString("Handler is null");
        return ERR_FAILED;
    }
    this->handler->setFlags(Flags);
    return STATUS_NOERROR;
}

int channel::setBaud(unsigned long Baudrate)
{
    if (this->handler == nullptr) {
        globals::setErrorString("Handler is null");
        return ERR_FAILED;
    }
    this->handler->setBaud(Baudrate);
    return STATUS_NOERROR;
}

int channel::setMacchinaChannel()
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
    CMD_RES res = usbcomm::sendMsgResp(&m);
    if (res == CMD_RES::CMD_FAIL) {
        return m.args[1];
    }
    else if (res == CMD_RES::CMD_TIMEOUT) {
        // Copy comm error msg
        globals::setErrorString(usbcomm::getLastError());
        return ERR_FAILED;
    }
    return STATUS_NOERROR;
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
    usbcomm::sendMsg(&m);
    return 0;
}

int channel::setFilter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID)
{
    // Safety test - if filter is 0x03, then pFlowControlMsg must NOT be null as laid out in spec!
    if (FilterType == FLOW_CONTROL_FILTER && pFlowControlMsg == nullptr) {
        LOGGER.logError("CHAN_FILT", "Flow control filter wanted but pFlowControlMsg is null!");
        return ERR_NULL_PARAMETER;
    }

    // Check if we have avaliable channel filters
    for (int i = 0; i < CHANNEL_MAX_FILTERS; i++) {
        if (filters[i] == nullptr) { // Found a free slot, allocate it
            filters[i] = new handler_filter{ 0x00 };

            // Set filter values
            filters[i]->id = i+1;
            *pFilterID = (unsigned long)(i + 1);
            filters[i]->type = (uint8_t)FilterType;
            memcpy(&filters[i]->mask, pMaskMsg, sizeof(&pMaskMsg));
            memcpy(&filters[i]->filter, pPatternMsg, sizeof(&pPatternMsg));
            if (FilterType == FLOW_CONTROL_FILTER) { // Only copy if flow control (else pFlowControl is nullptr)
                memcpy(&filters[i]->flow, pFlowControlMsg, sizeof(&pFlowControlMsg));
            }

            // Construct data to send to Macchina device
            PCMSG m = { 0x00 };
            m.cmd_id = CMD_CHANNEL_SET_FILTER;
            m.arg_size = 15; // 1 for CID, 1 for FID, 1 for FType, 4 for Mask, 4 for pattern, 4 for Flow
            m.args[0] = this->id; // ID of channel for the filter
            m.args[1] = filters[i]->id; // Filter ID to set on Macchina
            m.args[2] = filters[i]->type; // Type of filter
            // Copy the first 4 the bytes for each filter, the rest we can do in Software later
            memcpy(&m.args[3], &pMaskMsg->Data[0], 4);
            memcpy(&m.args[7], &pPatternMsg->Data[0], 4);
            if (FilterType == FLOW_CONTROL_FILTER) {
                memcpy(&m.args[11], &pFlowControlMsg->Data[0], 4);
            }
            usbcomm::sendMsg(&m);
            LOGGER.logDebug("CAN_FILT", "Adding filter with ID %lu", *pFilterID);
            return STATUS_NOERROR;
        }
    }
    // No more free filters
    LOGGER.logError("CAN_FILT", "Cannot add any more filters - Limit exceeded");
    return ERR_EXCEEDED_LIMIT;
}

int channel::remove_filter(unsigned long filterID)
{
    // Filter doesn't exit?
    if (filters[filterID - 1] == nullptr) {
        LOGGER.logError("CAN_FILT", "Cannot remove filter with ID of %lu, does not exist!", filterID);
        return ERR_INVALID_MSG_ID;
    }
    LOGGER.logDebug("CAN_FILT", "Removing filter with ID %lu", filterID);
    // Filter exists, remove it
    delete filters[filterID - 1];
    filters[filterID - 1] = nullptr;
    PCMSG m = { 0x00 };
    m.cmd_id = CMD_CHANNEL_REM_FILTER;
    m.arg_size = 2; // 1 for CID, 1 for FID
    m.args[0] = this->id;
    m.args[1] = filterID;
    usbcomm::sendMsg(&m);
    return STATUS_NOERROR;
}

int channel::removeChannel()
{
    PCMSG m = {
        CMD_CHANNEL_DESTROY,
        1,
    };
    m.args[0] = this->id;
    // Ensure Macchina removed the channel

    switch (usbcomm::sendMsgResp(&m))
    {
    case CMD_RES::CMD_OK:
        return STATUS_NOERROR;
    case CMD_RES::SEND_FAIL:
        return ERR_DEVICE_NOT_CONNECTED;
    case CMD_RES::CMD_FAIL:
        LOGGER.logError("CHAN_DEL", "Macchina failed to remove channel");
        return m.args[1];
    case CMD_RES::CMD_TIMEOUT:
        globals::setErrorString(usbcomm::getLastError());
        return ERR_FAILED;
    default:
        LOGGER.logError("CHAN_DEL", "WTF - CMD_RES invalid??");
        globals::setErrorString("CMD_RES invalid");
        return ERR_FAILED;
    }
}

void channel::recvData(uint8_t* m, uint16_t len)
{
    LOGGER.logDebug("CHAN_RECV","Incomming data for channel %d, size: %lu", this->id, len);
}

int channel::requestData(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
    // TODO - Read from buffer
    return ERR_BUFFER_EMPTY; // Temporary return value
}
