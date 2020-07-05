#include "pch.h"
#include "Logger.h"

void Logger::logInfo(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[INFO ] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logWarn(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[WARN ] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logError(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[ERROR] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logDebug(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[DEBUG] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

std::string Logger::argFormatToString(const char* fmt, va_list* args) {
	char buffer[4096] = { 0x00 };
	int rc = vsnprintf_s(buffer, sizeof(buffer), fmt, *args);
	return std::string(buffer);
}

void Logger::writeToFile(std::string message) {
	char time[64] = { 0x00 };
	SYSTEMTIME st;
	GetSystemTime(&st);
	sprintf_s(time, "[%02d:%02d:%02d.%3d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	std::ofstream handle;
	this->mutex.lock();
	try {
		handle.open(LOG_FILE, std::ios_base::app);
		handle << time << message << "\n" << std::flush;
		handle.close();
	}
	catch (std::ofstream::failure e) {
		//TODO handle error
	}
	this->mutex.unlock();
}

std::string Logger::bytesToString(uint8_t* bytes, unsigned long len) {
	std::string ret = "";
	LOGGER.logDebug("LB", "%lu bytes", len);
	char buf[4] = { 0x00 };
	for (int i = 0; i < len; i++) {
		sprintf(buf, "%02X ", bytes[i]);
		ret += buf;
	}
	return ret;
}

Logger LOGGER;