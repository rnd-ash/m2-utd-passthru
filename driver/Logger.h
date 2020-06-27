#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <Windows.h>
#include <mutex>

#define LOG_FILE "C:\\Program Files (x86)\\macchina\\passthru\\activity.log"

class Logger
{
private:
	std::mutex mutex;
	std::string argFormatToString(const char* fmt, va_list* args);
public:
	void logInfo(std::string method, const char* fmt, ...);
	void logWarn(std::string method, const char* fmt, ...);
	void logError(std::string method, const char* fmt, ...);
	void logDebug(std::string method, const char* fmt, ...);
	void writeToFile(std::string message);
};

extern Logger LOGGER;

