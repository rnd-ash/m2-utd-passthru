#pragma once

#include <stdint.h>
#include <string>

#ifndef GLOBALS_H_
#define GLOBALS_H_

// Contains values that are updated at a ping request to the macchina
namespace globals
{
	void setBatVoltage(unsigned long v);
	unsigned long getBatVoltage();
	void setErrorString(std::string error);
	void setErrorString(const char* error);
	std::string getErrorString();
};

#endif

