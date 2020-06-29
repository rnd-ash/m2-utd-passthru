#include "pch.h"
#include "macchina-passthru.h"
#include "Logger.h"
#include "usbcomm.h"
#include "globals.h"
#include "channel.h"


char lastError[100];

/*
http://www.drewtech.com/support/passthru/open.html
Establish a logical communication channel with the vehicle network (via the PassThru device) using the specified network layer protocol and selected protocol options.
*/
DllExport PassThruOpen(void* pName, unsigned long* pDeviceID) {
	LOGGER.logInfo("DllExport", "PassThruOpen called");
	*pDeviceID = 1L;
	return STATUS_NOERROR;
}

/*
http://www.drewtech.com/support/passthru/close.html
Close all communication with the PassThru device. All channels will be disconnected from the network,
periodic messages will halt, and the hardware will return to its default state.
*/
DllExport PassThruClose(unsigned long DeviceID) {
	LOGGER.logInfo("DllExport", "PassThruClose called");
	return STATUS_NOERROR;
}

/*
http://www.drewtech.com/support/passthru/connect.html
Establish a logical communication channel with the vehicle network (via the PassThru device) using the specified network layer protocol and selected protocol options.
*/
DllExport PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long* pChannelID) {
	LOGGER.logInfo("DllExport", "PassThruConnect called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	unsigned long id = channels.addChannel(ProtocolID, Flags, Baudrate);
	if (id == 0) { // Error - copy it to our error buffer
		strcpy_s(lastError, "No more avaliable channels");
		return ERR_FAILED;
	}
	else { // OK! Copy ID back to app
		*pChannelID = id;
		return STATUS_NOERROR;
	}
}

/*
http://www.drewtech.com/support/passthru/disconnect.html
Terminate an existing logical communication channel between the User Application and the vehicle network (via the PassThru device).
Once disconnected the channel identifier or handle is invalid. For the associated network protocol this function will terminate
the transmitting of periodic messages and the filtering of receive messages. The PassThru device periodic and filter message tables
will be cleared.
*/
DllExport PassThruDisconnect(unsigned long ChannelID) {
	LOGGER.logInfo("DllExport", "PassThruDisconnect called - Channel is %lu", ChannelID);
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	return channels.removeChannel(ChannelID);
}

/*
http://www.drewtech.com/support/passthru/readmsgs.html
Receive network protocol messages, receive indications, and transmit indications from an existing logical communication channel.
Messages will flow through PassThru device to the User Application..
*/
DllExport PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout) {
	//LOGGER.logInfo("DllExport", "PassThruReadMsgs called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	return STATUS_NOERROR;
}

/*
http://www.drewtech.com/support/passthru/writemsgs.html
Transmit network protocol messages over an existing logical communication channel. Messages will flow through PassThru device to the vehicle network.
*/
DllExport PassThruWriteMsgs(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout) {
	LOGGER.logInfo("DllExport", "PassThruWriteMsgs called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	return channels.send_payload(ChannelID, pMsg, pNumMsgs, Timeout);
}

/*
http://www.drewtech.com/support/passthru/startperiodicmsg.html
Repetitively transmit network protocol messages at the specified time interval over an existing logical communication channel.
There is a limit of ten periodic messages per network layer protocol.
*/
DllExport PassThruStartPeriodicMsg(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pMsgID, unsigned long TimeInterval) {
	LOGGER.logInfo("DllExport", "PassThruStartPeriodicMsg called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	return STATUS_NOERROR;
}

/*
http://www.drewtech.com/support/passthru/stopperiodicmsg.html
Terminate the specified periodic message. Once terminated the message identifier or handle value is invalid
*/
DllExport PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID) {
	LOGGER.logInfo("DllExport", "PassThruStopPeriodicMsg called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	return STATUS_NOERROR;
}

/*
http://www.drewtech.com/support/passthru/startmsgfilter.html
The PassThruStartMsgFilter function is used to setup a network protocol filter that will selectively restrict or
limit network protocol messages received by the PassThru device. The filter messages will flow from the User Application
to the PassThru device. There is a limit of ten filter messages per network layer protocol.
The PassThru device will block all vehicle network receive frames by default, when no filters are defined.
The CLEAR_RX_BUFFER (PassThruIoctl function) command must be used after establishing filters to ensure that the receive
queue only contains receive frames that adhere to the filter criteria. The PassThruStartMsgFilter function does not cause
existing receive messages to be removed from the PassThru device receive queue.
*/
DllExport PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID) {
	LOGGER.logInfo("DllExport", "PassThruStartMsgFilter called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	return channels.setFilter(ChannelID, FilterType, pMaskMsg, pPatternMsg, pFlowControlMsg, pFilterID);
}

/*
http://www.drewtech.com/support/passthru/stopmsgfilter.html
Terminate the specified network protocol filter. Once terminated the filter identifier or handle value is invalid.
*/
DllExport PassThruStopMsgFilter(unsigned long ChannelID, unsigned long FilterID) {
	LOGGER.logInfo("DllExport", "PassThruStopMsgFilter called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	return channels.remove_filter(ChannelID, FilterID);
}

/*
http://www.drewtech.com/support/passthru/setprogramming.html
Output a programmable voltage on the specified J1962 connector pin.
Only one pin can have a specified voltage applied at a time. The only exception: it is permissible to
program pin 15 for SHORT_TO_GROUND, and another pin to a voltage level.
When switching pins, the user application must disable the first voltage (VOLTAGE_OFF option)
before enabling the second. The user application protect against applying any incorrect voltage levels.
A current in excess of 200mA will damage CarDAQ; do not ground the FEPS line while energized, even briefly.
*/
DllExport PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long PinNumber, unsigned long Voltage) {
	LOGGER.logInfo("DllExport", "PassThruSetProgrammingVoltage called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	return STATUS_NOERROR;
}

/*
http://www.drewtech.com/support/passthru/readversion.html
Retrieve the PassThru device firmware version, the PassThru device DLL version,
and the version of the J2534 specification that was referenced. The version information is in the form of NULL terminated strings.
*/
DllExport PassThruReadVersion(unsigned long DeviceID, char* pFirmwareVersion, char* pDllVersion, char* pApiVersion) {
	LOGGER.logInfo("DllExport", "passThruReadVersion called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	memcpy(pFirmwareVersion, FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION));
	memcpy(pDllVersion, DLL_VERSION, sizeof(DLL_VERSION));
	memcpy(pApiVersion, API_VERSION, sizeof(API_VERSION));
	return STATUS_NOERROR;
}

/*
http://www.drewtech.com/support/passthru/getlasterror.html
Retrieve a text description for the most recent PassThru error as a null terminated C-string. Call this function immediately after an error occurs.
The error string refers to the most recent function call, rather than a specific DeviceID or ChannelID, and any subsequent function call may clobber the description.
*/
DllExport PassThruGetLastError(char* pErrorDescription) {
	LOGGER.logInfo("DllExport", "PassThruGetLastError called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	if (pErrorDescription == nullptr) {
		LOGGER.logError("DllExport", "Error description is a null pointer!?");
		return ERR_NULL_PARAMETER;
	}
	memcpy(pErrorDescription, lastError, strlen(lastError));
	return STATUS_NOERROR;
}

/*
http://www.drewtech.com/support/passthru/ioctl.html
The PassThruIoctl function is a general purpose I/O control function for modifying the vehicle network interface's characteristics.
*/
DllExport PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, void* pInput, void* pOutput) {
	LOGGER.logInfo("DllExport", "PassThruIOCTL called");
	if (!usbcomm::isConnected()) {
		return ERR_DEVICE_NOT_CONNECTED;
	}
	// Test - Just copy voltage
	if (IoctlID == READ_VBATT) {
		*(unsigned long*)pOutput = globals::getBatVoltage();
	}
	return STATUS_NOERROR;
}
