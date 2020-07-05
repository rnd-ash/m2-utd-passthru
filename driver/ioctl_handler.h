#pragma once

#ifndef IOCTL_H_
#define IOCTL_H_

#include "j2534_v0404.h"

namespace ioctl_handler
{
	int set_config(unsigned long channelID, SCONFIG_LIST* pInput);
	int get_config(unsigned long channelID, SCONFIG_LIST* pInput);
	int read_batt(unsigned long* vbatt);
	int read_prog_voltage(unsigned long* vProg);
	int five_baud_init(unsigned long channelID, SBYTE_ARRAY* pInput, SBYTE_ARRAY* pOutput);
	int fast_init(unsigned long channelID, PASSTHRU_MSG* pInput, PASSTHRU_MSG* pOutput);
	int clear_tx_buffers(unsigned long channelID);
	int clear_rx_buffers(unsigned long channelID);
	int clear_periodic_msgs(unsigned long channelID);
	int clear_msg_filters(unsigned long channelID);
	int clear_mlt(unsigned long channelID);
	int add_to_mlt(unsigned long channelID, SBYTE_ARRAY* pInput);
	int del_from_mlt(unsigned long channelID, SBYTE_ARRAY* pInput);

};

#endif

