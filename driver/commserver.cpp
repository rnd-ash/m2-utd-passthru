#include "pch.h"
#include "commserver.h"
#include "Logger.h"
#include "usbcomm.h"
#include "globals.h"

namespace commserver {
	HANDLE thread = NULL; // Comm thread
	HANDLE pingThread = NULL;
	HANDLE askInitEvent; // Handle for if other threads want to init
	HANDLE commEvent; // Handle for event from arduino
	HANDLE exitEvent; // Handle for exiting / closing thread
	HANDLE closedEvent; // Handle for when thread is closed
	HANDLE events[20];
	PCMSG d = { 0x00 };
	bool can_read = false;
	int eventCount = 0;


	void CloseHandles() {
		CloseHandle(askInitEvent);
		CloseHandle(exitEvent);
		CloseHandle(commEvent);
		CloseHandle(closedEvent);
	}

	int WaitUntilReady(const char* deviceName, unsigned long timeout) {
		if (usbcomm::isConnected()) {
			return 0;
		}
		else {
			LOGGER.logInfo("commserver::Wait", "Waiting for Macchina");
			const clock_t begin_time = clock();
			while (clock() - begin_time / (CLOCKS_PER_SEC / 1000) <= timeout) {
				if (usbcomm::OpenPort()) {
					LOGGER.logInfo("commserver::Wait", "Macchina ready!");
					return 0;
				}
			}
			LOGGER.logError("commserver::Wait", "Macchina timeout error!");
		}
		return 1;
	}

	void CloseCommThread() {
		LOGGER.logInfo("commserver::CloseCommThread", "Closing comm thread");
		can_read = false;
		// Send one more thing to macchina letting it know driver is quitting
		d.cmd_id = CMD_EXIT;
		usbcomm::sendMessage(&d);

		WaitForSingleObject(closedEvent, 5000); // Wait for 5 seconds for the thread to terminate
		CloseHandles();
		CloseHandle(thread);
		CloseHandle(pingThread);
	}

	bool CreateEvents() {
		askInitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (askInitEvent == NULL) {
			LOGGER.logWarn("commserver::CreateEvents", "Cannot create init event!");
			return false;
		}

		exitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (exitEvent == NULL) {
			LOGGER.logWarn("commserver::CreateEvents", "Cannot create exit event!");
			return false;
		}
		closedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (closedEvent == NULL) {
			LOGGER.logWarn("commserver::CreateEvents", "Cannot create closed event!");
			return false;
		}
		commEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (commEvent == NULL) {
			LOGGER.logWarn("commserver::CreateEvents", "Cannot create comm event!");
			return false;
		}

		events[0] = askInitEvent;
		events[1] = commEvent;
		events[2] = exitEvent;
		eventCount = 3;
		return true;
	}

	bool waitForEvents() {
		DWORD ret;
		ret = WaitForMultipleObjects(eventCount, events, false, INFINITE);
		// Event[0] = Init
		// Event[1] = Comm
		// Event[2] = Exit
		if (ret == (WAIT_OBJECT_0 + 0)) {
			LOGGER.logInfo("commserver::waitForEvents", "Init event handled");
			// TODO Handle Init event
		}
		else if (ret == (WAIT_OBJECT_0 + 1)) {
			LOGGER.logInfo("commserver::waitForEvents", "Communication event handled");
			// TODO Handle Communication event
		}
		else if (ret == (WAIT_OBJECT_0 + 2)) {
			LOGGER.logInfo("commserver::waitForEvents", "Exit event handled");
			// TODO Handle exit event
			return false;
		}
		else {
			LOGGER.logInfo("commserver::waitForEvents", "Unknown handle!");
			return false;
		}
		return true;
	}

	void pingMacchina() {
		PCMSG send = { 0x0F };
		send.cmd_id = CMD_PING;
		send.arg_size = 0x02;
		send.args[0] = 0x01;
		send.args[0] = 0x02;
		send.args[0] = 0x03;
		usbcomm::sendMessage(&send);
	}
	
	void processPingResponse(PCMSG *msg) {
		// Reponse args:
		// 0-3 = Batter voltage (mV)
		// 4 - Current number of channels open
		float bat;
		memcpy(&bat, &msg->args[0], 4);
		uint8_t channel_count = msg->args[4];
		globals::setBatVoltage((unsigned long)(bat * 1000)); // Go back to mV (Macchina sends it in V)
		LOGGER.logDebug("MACCHINA-PING", "PING - Battery voltage %f v, %d active channels", bat, channel_count);
	}

	DWORD WINAPI PingLoop() {
		while (can_read && usbcomm::isConnected()) { // Stop pinging on disconnect
			pingMacchina();
			// Ping every second, so sleep here
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
		return 0;
	}

	DWORD WINAPI CommLoop() {
		d.arg_size = 500;
		d.cmd_id = 0x05;
		while (can_read) {
			// Message received from Macchina
			if (usbcomm::pollMessage(&d)) {
				// Ping - Process and go back to top of loop
				if (d.cmd_id == CMD_PING) { // Its a ping message!
					processPingResponse(&d);
					continue;
				}
				// TODO Process payloads
			}
		}
		return 0;
	}

	DWORD WINAPI startCommPing(LPVOID lpParm) {
		LOGGER.logInfo("commserver::startPingComm", "started!");
		PingLoop();
		LOGGER.logInfo("commserver::startPingComm", "Exiting!");
		return 0;
	}

	DWORD WINAPI startComm(LPVOID lpParam) {
		LOGGER.logInfo("commserver::startComm", "started!");
		can_read = true;
		CommLoop();
		// TODO Handle driver upon exit
		LOGGER.logInfo("commserver::startComm", "Exiting!");
		SetEvent(closedEvent);
		return 0;
	}

	bool CreateCommThread() {
		// Check if thread is already running
		if (thread == NULL) {
			LOGGER.logInfo("commserver::CreateCommThread", "Creating events for thread");
			if (!CreateEvents()) {
				LOGGER.logError("commserver::CreateCommThread", "Failed to create events!");
				return false;
			}
			LOGGER.logInfo("commserver::CreateCommThread", "Creating thread");
			thread = CreateThread(NULL, 0, startComm, NULL, 0, NULL);
			pingThread = CreateThread(NULL, 0, startCommPing, NULL, 0, NULL);
			if (thread == NULL) {
				LOGGER.logError("commserver::CreateCommThread", "Recv Thread could not be created!");
				return false;
			}
			if (pingThread == NULL) {
				LOGGER.logError("commserver::CreateCommThread", "Ping Thread could not be created!");
				return false;
			}
			LOGGER.logInfo("commserver::CreateCommThread", "Threads created!");
		}
		if (WaitUntilReady("", 3000) != 0) {
			LOGGER.logInfo("commserver::CreateCommThread", "Macchina is not avaliable!");
			return false;
		}
		return true;
	}
}