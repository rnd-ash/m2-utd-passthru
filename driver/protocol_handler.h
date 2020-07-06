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

#pragma once
#include <queue>
#include "j2534_v0404.h"

class protocol_handler
{
public:
	explicit protocol_handler(unsigned long channelID);
	void setFlags(unsigned long flags);
	void setBaud(unsigned long baud);
	unsigned long getBaud();
	virtual void recvData(uint8_t* m, uint16_t len) = 0;
	int requestData(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout);
protected:
	std::queue<PASSTHRU_MSG> msg_queue;
	unsigned long baud;
	unsigned long flags;
	unsigned long channelid;
};

class iso9141_handler : public protocol_handler {
public:
	iso9141_handler(unsigned long channelID);
	void recvData(uint8_t* m, uint16_t len);
};

class iso15765_handler : public protocol_handler {
public:
	iso15765_handler(unsigned long channelID);
	void recvData(uint8_t* m, uint16_t len);
};

class can_handler : public protocol_handler {
public:
	can_handler(unsigned long channelID);
	void recvData(uint8_t* m, uint16_t len);
};

