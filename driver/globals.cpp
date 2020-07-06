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
#include "globals.h"

namespace globals {
	unsigned long BAT_VOLTAGE = 12000; // 12.0 V as default - macchina will update on ping
	std::string lastErrorMsg = "NO ERROR";

	void setBatVoltage(unsigned long v)
	{
		BAT_VOLTAGE = v;
	}

	unsigned long getBatVoltage()
	{
		return BAT_VOLTAGE;
	}
	void setErrorString(std::string error)
	{
		lastErrorMsg = error;
	}
	void setErrorString(const char* error)
	{
		lastErrorMsg.assign(error, strlen(error));
	}
	std::string getErrorString()
	{
		return lastErrorMsg;
	}
}
