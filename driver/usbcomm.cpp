#include "pch.h"
#include "usbcomm.h"
#include <mutex>
#include "Logger.h"

namespace usbcomm {
	HANDLE handler;
	bool connected = false;
	std::mutex mutex;
	COMSTAT com;
	DWORD errors;
	std::string lastError = "";

	// For thread-safe command result processing
	bool hasResult = false;
	PCMSG lastResult = { 0x00 };
	std::mutex resMutex;

	bool OpenPort() {
		mutex.lock();
		// TODO - Allow different COM Ports
		handler = CreateFile(L"\\\\.\\COM12", GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (handler == INVALID_HANDLE_VALUE) {
			LOGGER.logError("MACCHINA", "Cannot create handler - error is %d", GetLastError());
			mutex.unlock();
			return false;
		}

		DCB params = { 0x00 };
		if (!GetCommState(handler, &params)) {
			LOGGER.logError("MACCHINA", "Cannot read comm states - error is %d", GetLastError());
			mutex.unlock();
			return false;
		}

		params.BaudRate = CBR_115200;
		params.ByteSize = 8;
		params.StopBits = ONESTOPBIT;
		params.Parity = NOPARITY;
		params.fDtrControl = DTR_CONTROL_DISABLE;

		if (!SetCommState(handler, &params)) {
			LOGGER.logError("MACCHINA", "Cannot set comm states - error is %d", GetLastError());
			mutex.unlock();
			return false;
		}

		PurgeComm(handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
		connected = true;
		mutex.unlock();
		return true;
	}

	void ClosePort() {
		mutex.lock();
		CloseHandle(handler);
		mutex.unlock();
		connected = false;
	}

	std::string getLastError()
	{
		return lastError;
	}

	bool internalSendMessage(PCMSG* msg) {
		DWORD written = 0;
		mutex.lock();
		if (!WriteFile(handler, msg, sizeof(struct PCMSG), &written, NULL)) {
			DWORD error = GetLastError();
			LOGGER.logWarn("M_SEND", "Error writing message! Code %d", (int)error);
			if (error == 22 || error == 433) { // Device doesn't exit!? - Maybe unplugged!
				connected = false;
			}
			mutex.unlock();
			return false;
		}
		mutex.unlock();
		return true;
	}

	bool sendMsg(PCMSG* msg) {
		return sendMsgResp(msg, 0) == CMD_RES::CMD_OK;
	}

	CMD_RES sendMsgResp(PCMSG* msg) {
		return sendMsgResp(msg, 10); // Min wait is 10ms (Lets read thread do some processing)
	}

	CMD_RES sendMsgResp(PCMSG* msg, unsigned long maxWaitMs)
	{
		hasResult = false; // Set this here, so this is only True when the reader thread detects a response
		// Send the message first
		if (!internalSendMessage(msg)) {
			lastError = "Could not send command to Macchina";
			return CMD_RES::SEND_FAIL;
		}
		// Don't need a response, who cares
		if (maxWaitMs == 0) {
			return CMD_RES::CMD_OK;
		}
		// Wait for our response message
		const clock_t begin_time = clock();
		while (!hasResult && clock() - begin_time / (CLOCKS_PER_SEC / 1000) <= maxWaitMs) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		// Either timer expired or we have a message before timer ended, figure out which
		if (!hasResult) { // Still no result!? - Macchina timeout
			LOGGER.logError("M_SEND_RESP", "Timeout waiting for Macchina to respond", lastError.c_str());
			lastError = "Timeout requesting response";
			return CMD_RES::CMD_TIMEOUT;
		}

		// Yay - We have a response from Macchina - process it
		CMD_RES return_result = CMD_RES::CMD_FAIL;
		resMutex.lock(); // Lock result mutex
		if (lastResult.cmd_id == msg->cmd_id) {
			memcpy(msg, &lastResult, sizeof(struct PCMSG)); // copy the response back
			if (lastResult.args[0] == CMD_RES_STATE_OK) {
				LOGGER.logDebug("M_SEND_RESP", "Macchina responded OK!");
				return_result = CMD_RES::CMD_OK; // Command OK!
			}
			else if (lastResult.args[0] == CMD_RES_STATE_FAIL) { // Oh no! Command failed
				lastError.assign(lastResult.args[2], lastResult.arg_size-2); // String starts at pos 2 in the result // 0 - Result, 1 - Result code
				LOGGER.logError("M_SEND_RESP", "Macchina failed to process request. Message: '%s'", lastError.c_str());
				return_result = CMD_RES::CMD_FAIL;
			}
		}
		else {
			// WTF - Macchina responded with the WRONG CMD ID (Maybe its for a different command?)
			LOGGER.logError("M_SEND_RESP", "Macchina responded with wrong CMD ID. Want: %02X, got %02X", msg->cmd_id, lastResult.cmd_id);
			lastError = "Macchina responded with result for wrong command";
			return_result = CMD_RES::CMD_TIMEOUT;
		}
		resMutex.unlock(); // Unlock result mutex

		// Response found! - Query it
		return return_result;
	}


	bool pollMessage(PCMSG* msg) {
		if (!connected) { // Don't throw an exception, exit early if not connected
			return false;
		}
		DWORD read = 0;
		mutex.lock();
		ClearCommError(handler, &errors, &com);
		memset(msg, 0x00, sizeof(struct PCMSG));
		if (com.cbInQue >= sizeof(struct PCMSG)) {
			ReadFile(handler, msg, sizeof(struct PCMSG), &read, NULL);
			mutex.unlock();
			if (read != sizeof(struct PCMSG)) {
				LOGGER.logError("M_READ", "Missmatch. Want %lu bytes, got %lu", sizeof(PCMSG), read);
				return false;
			}
			if (msg->cmd_id == CMD_LOG) {
				LOGGER.logInfo("M_READ", "Macchina message: '%s'", msg->args);
				return false;
			}
			// Its a response message for a command sent on another thread!
			else if ((msg->cmd_id & 0xF0) == CMD_RES_FROM_CMD) {
				LOGGER.logDebug("M_READ", "Received a result message");
				resMutex.lock();
				memcpy(&lastResult, msg, sizeof(struct PCMSG));
				lastResult.cmd_id &= 0x0F; // Remove the extra 0xA from the cmd
				hasResult = true;
				resMutex.unlock();
				return false; // Return false so we don't process it later on this thread
			}
			return true;
		}
		mutex.unlock();
		return false;
	}

	bool isConnected() {
		return connected;
	}
}