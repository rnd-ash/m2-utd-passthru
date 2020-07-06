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
#include "usbcomm.h"
#include <mutex>
#include <map>
#include "Logger.h"

namespace usbcomm {
	HANDLE handler;
	bool connected = false;
	std::mutex mutex;
	COMSTAT com;
	DWORD errors;
	std::string lastError = "";
	


	uint8_t msg_id = 0x01;
	std::map<uint8_t, PCMSG> results;
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

	bool internalSendMsg(PCMSG* msg, bool responseRequired) {
		msg->__require_response = responseRequired; // Just for sanity sake
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

	bool mapHasResult(uint8_t wantID) {
		return results.find(wantID) != results.end();
	}

	bool sendMsg(PCMSG* msg) {
		return internalSendMsg(msg, false);
	}

	CMD_RES sendMsgResp(PCMSG* msg, PCMSG* resp)
	{
		resMutex.lock();
		results.erase(msg_id); // Erase any old message that was in this ID's slot
		uint8_t want_id = msg_id; // Set the target ID to the msg_id
		msg_id++; // Incriment the next id for future sent message
		msg->msg_id = want_id; // Set it in the message so Macchina knows it has to respond with same ID
		resMutex.unlock();
		// Send the message first
		if (!internalSendMsg(msg, true)) {
			lastError = "Could not send command to Macchina";
			return CMD_RES::SEND_FAIL;
		}
		// Wait for our response message
		const clock_t begin_time = clock();
		while (!mapHasResult(want_id) && clock() - begin_time / (CLOCKS_PER_SEC / 1000) <= MAX_WAIT_TIME_MS) {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

		// Either timer expired or we have a message before timer ended, figure out which
		if (!mapHasResult(want_id)) { // Still no result!? - Macchinas probably frozen (again!)
			LOGGER.logError("M_SEND_RESP", "Timeout waiting for Macchina to respond", lastError.c_str());
			lastError = "Timeout requesting response";
			return CMD_RES::CMD_TIMEOUT;
		}

		resMutex.lock(); // Lock damn result mutex so poller thread can't touch the results map whilst im doing memcpy (This caused 8+ hours of pain...!)
		PCMSG p = results.at(want_id);
		memcpy(&resp, &p, sizeof(struct PCMSG)); // Copies the found message to msg pointer
		results.erase(want_id); // We can now remove the message in the results map
		resMutex.unlock(); // Unlock this piece of shit....Not optimal but multithreading is making me loosing my sanity here

		if (resp->resp_code == STATUS_NOERROR) { // Macchina happily responded to the request sent
			return CMD_RES::CMD_OK;
		}
		else { // FFS. Something happened on Macchina, report the error (Args are the error string)
			lastError.assign((char*)msg->args, msg->arg_size);
			LOGGER.logDebug("M_SEND_RESP", "Macchina Failed to process request. Error: '%s'", lastError);
			return CMD_RES::CMD_FAIL;
		}
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
				//LOGGER.logDebug("M_READ", "Received a result message - ID %02X, Code: %02X", msg->msg_id, msg->resp_code);
				resMutex.lock();
				PCMSG tmp = {0x00};
				memcpy(&tmp, msg, sizeof(struct PCMSG));
				results.emplace(msg->msg_id, tmp);
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