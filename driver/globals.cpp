#include "pch.h"
#include "globals.h"


namespace globals {
	unsigned long BAT_VOLTAGE = 12000; // 12.0 V as default - macchina will update on ping
	void setBatVoltage(unsigned long v)
	{
		BAT_VOLTAGE = v;
	}
	unsigned long getBatVoltage()
	{
		return BAT_VOLTAGE;
	}
}