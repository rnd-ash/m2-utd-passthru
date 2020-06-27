#include "pch.h"
#include "commserver.h"
#include "Logger.h"
#include "usbcomm.h"

namespace commserver {
	HANDLE thread = NULL; // Comm thread
	HANDLE askInitEvent; // Handle for if other threads want to init
	HANDLE commEvent; // Handle for event from arduino
	HANDLE exitEvent; // Handle for exiting / closing thread
	HANDLE closedEvent; // Handle for when thread is closed
	HANDLE events[20];
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
					LOGGER.logError("commserver::Wait", "Macchina ready!");
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
		WaitForSingleObject(closedEvent, 5000); // Wait for 5 seconds for the thread to terminate
		CloseHandles();
		CloseHandle(thread);
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


	PCMSG d = { 0x00 };
	
	DWORD WINAPI CommLoop() {
		while (can_read) {
			if (usbcomm::pollMessage(&d)) {
				// TODO Process payloads
				LOGGER.logDebug("COMM_LOOP", "Payload read!");
			}
		}
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
			if (thread == NULL) {
				LOGGER.logError("commserver::CreateCommThread", "Thread could not be created!");
				return false;
			}
			LOGGER.logInfo("commserver::CreateCommThread", "Thread created!");
		}
		if (WaitUntilReady("", 3000) != 0) {
			LOGGER.logInfo("commserver::CreateCommThread", "Macchina is not avaliable!");
			return false;
		}
		return true;
	}
}