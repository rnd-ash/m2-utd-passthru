#pragma once

#include <stdint.h>
#include <string>
#include "j2534_v0404.h"

#define MAX_WAIT_TIME_MS 2000

// Command ID's for Misc
#define CMD_LOG  0x01
#define CMD_PING 0x02

// Channel command ID's
#define CMD_CHANNEL_CREATE     0x03 // Creating a new channel
#define CMD_CHANNEL_DATA       0x04 // Send data to/from channel
#define CMD_CHANNEL_DESTROY    0x05 // Killing a channel
#define CMD_CHANNEL_IOCTL_REQ  0x06 // IOCTL request to device
#define CMD_CHANNEL_IOCTL_RESP 0x07 // IOCTL Response from device
#define CMD_CHANNEL_SET_FILTER 0x08 // Add a filter to a channel
#define CMD_CHANNEL_REM_FILTER 0x09 // Remove a filter from a channel;

// Command responses (From macchina)
#define CMD_RES_FROM_CMD       0xA0 // This gets put onto the first nibble of a CMD Id if its the Macchina responding from it 

#define CMD_EXIT 0xFF // If sent, device will reset itself back into default state (Assume use app has quit)


/// <summary>
/// Structure that is transmitted to and from the Macchina
/// </summary>
struct PCMSG { // Total 512 bytes
    uint8_t cmd_id;
    uint8_t resp_code; // J2534 response code
    uint16_t arg_size;
    uint8_t args[512];
    uint8_t msg_id;
    bool __require_response;
};

/// <summary>
/// Enumeration for Macchina response codes
/// </summary>
enum class CMD_RES {
    // Send OK - And Macchina was OK processing request
    CMD_OK,
    // Send Failed (Macchina didn't even receive the request)
    SEND_FAIL,
    // Macchina failed to process the command sent to it
    CMD_FAIL,
    // Macchina did not respond in time
    CMD_TIMEOUT
};

namespace usbcomm
{
    /// <summary>
    /// Polls for a message from Macchina
    /// </summary>
    /// <param name="msg">Pointer to a PCMSG that will be used if read is OK</param>
    /// <returns>Boolean indicating if data was read or not</returns>
    bool pollMessage(PCMSG* msg);

    /// <summary>
    /// Sends a message to Macchina, and doesn't poll for its response
    /// </summary>
    /// <param name="msg">Pointer to message to send</param>
    /// <returns>Boolean indicating if Message was sent OK to Macchina</returns>
    bool sendMsg(PCMSG* msg);

    /// <summary>
    /// Sends a message to Macchina, and attempts to read the response.
    /// This function will return CMD_TIMEOUT if a response is not seen after 2 seconds
    /// </summary>
    /// <param name="msg">Pointer to message to send</param>
    /// <param name="maxWaitMs">Maximum time in MS to poll for the response</param>
    /// <returns></returns>
    CMD_RES sendMsgResp(PCMSG* send, PCMSG* resp);

    /// <summary>
    /// Indicates if Macchina is currently connected or not
    /// </summary>
    /// <returns>Boolean indicating connection state</returns>
    bool isConnected();

    /// <summary>
    /// Attempts to open a Serial port connection to Macchina
    /// </summary>
    /// <returns>Boolean indicating if port was successfully opened</returns>
    bool OpenPort();

    /// <summary>
    /// Closes the port with the Macchina
    /// </summary>
    void ClosePort();

    /// <summary>
    /// Gets a string containing the last error message during message
    /// transmission, in the event of CMD_RES::CMD_FAIL, the error message
    /// originates from the Macchina itself
    /// </summary>
    /// <returns>String containing the last error message</returns>
    std::string getLastError();
};

