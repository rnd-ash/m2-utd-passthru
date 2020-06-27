#pragma once

#include <stdint.h>

// Contains values that are updated at a ping request to the macchina
namespace globals
{
	void setBatVoltage(unsigned long v);
	unsigned long getBatVoltage();
};

