/*
**
** Copyright (C) 2020 Ashcon Mohseninia
** Author: Ashcon Mohseninia <ashcon50@gmail.com>
**
** This library is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or (at
** your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, <http://www.gnu.org/licenses/>.
**
*/

#include "pch.h"
#include "protocol_handler.h"
#include "Logger.h"
#include "usbcomm.h"

protocol_handler::protocol_handler(unsigned long channelID)
{
	this->channelid = channelID;
	this->baud = 0;
	this->flags = 0;
}

void protocol_handler::setFlags(unsigned long flags)
{
	this->flags = flags;
}

void protocol_handler::setBaud(unsigned long baud)
{
	this->baud = baud;
}

unsigned long protocol_handler::getBaud()
{
	return this->baud;
}

int protocol_handler::requestData(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	if (this->msg_queue.size() == 0) { // Sorry, nothing here to read
		return ERR_BUFFER_EMPTY;
	}
	// TODO handle timeout
	unsigned long max_read = min(*pNumMsgs, msg_queue.size()); // Calculate how many messages we can read at most
	*pNumMsgs = max_read; // Let the req application know how many messages we read
	LOGGER.logDebug("HANDLER", "Sending %lu messages from Channel %lu back to app", max_read, this->channelid);
	for (unsigned long i = 0; i < max_read; i++) {
		memcpy(&pMsg[i], &this->msg_queue.front(), sizeof(PASSTHRU_MSG));
		LOGGER.logDebug("HANDLER", "Contents: %s", LOGGER.bytesToString(pMsg[i].Data, pMsg[i].DataSize).c_str());
		// Now pop the queue
		this->msg_queue.pop();
	}
	return STATUS_NOERROR;
}

iso9141_handler::iso9141_handler(unsigned long channelID) : protocol_handler(channelID)
{
	LOGGER.logDebug("ISO9141", "Handler created");
}

void iso9141_handler::recvData(uint8_t* m, uint16_t len)
{
	PASSTHRU_MSG rx = {};
	rx.ProtocolID = ISO9141;
	this->msg_queue.push(rx);
}

iso15765_handler::iso15765_handler(unsigned long channelID) : protocol_handler(channelID)
{
	LOGGER.logDebug("ISO15765", "Handler created");
}

void iso15765_handler::recvData(uint8_t* m, uint16_t len)
{
	// Now convert the data packet into a PASSTHRU_MSG
	PASSTHRU_MSG rx = { 0x00 };
	rx.ProtocolID = ISO15765;
	if (m[0] == 0xFF) { // Special indicator saying its a FIRST FF Indication
		LOGGER.logDebug("ISO15765", "First frame indication!");
		rx.DataSize = 4;
		rx.RxStatus = ISO15765_FIRST_FRAME; // Set this!
		memcpy(&rx.Data, &m[1], 4);
		this->msg_queue.push(rx);
	}
	else {
		LOGGER.logDebug("ISO15765", "Normal payload!");
		// Add the message to the queue
		rx.DataSize = len;
		rx.RxStatus = TX_MSG_TYPE; // Set this
		memcpy(&rx.Data, m, len);
		this->msg_queue.push(rx);
	}
}

can_handler::can_handler(unsigned long channelID) : protocol_handler(channelID)
{
	LOGGER.logDebug("CAN", "Handler created");
}

void can_handler::recvData(uint8_t* m, uint16_t len)
{
	PASSTHRU_MSG rx = { 0x00 };
	// Add the message to the queue
	rx.DataSize = len;
	rx.ProtocolID = CAN;
	memcpy(&rx.Data, m, len);
	this->msg_queue.push(rx);
}
