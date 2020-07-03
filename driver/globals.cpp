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
