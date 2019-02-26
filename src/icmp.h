#ifndef ICMP
#define ICMP
//********************************************************************************************
//
// File : icmp.h implement for Internet Control Message Protocol
//
//********************************************************************************************
//
// Copyright (C) 2007
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
// This program is distributed in the hope that it will be useful, but
//
// WITHOUT ANY WARRANTY;
//
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin St, Fifth Floor, Boston, MA 02110, USA
//
// http://www.gnu.de/gpl-ger.html
//
//********************************************************************************************

#define ICMP_TYPE_ECHOREPLY_V 	                0
#define ICMP_TYPE_ECHOREQUEST_V                 8
#define ICMP_TYPE_UNREACHABLE_V                 3
#define ICMP_CODE_UNREACHABLE_HOST_SERVICE_V    12

#define ICMP_HEADER_LEN         8
#define ICMP_MAX_DATA	        60

// icmp buffer position
#define ICMP_TYPE_P				0x22
#define ICMP_CODE_P				0x23
#define ICMP_CHECKSUM_H_P		0x24
#define ICMP_CHECKSUM_L_P		0x25
#define ICMP_IDENTIFIER_H_P		0x26
#define ICMP_IDENTIFIER_L_P		0x27
#define ICMP_SEQUENCE_H_P		0x28
#define ICMP_SEQUENCE_L_P		0x29
#define ICMP_DATA_P				(ICMP_TYPE_P + ICMP_HEADER_LEN)

//********************************************************************************************
//
// Prototype function
//
//********************************************************************************************
unsigned char icmp_send_reply ( unsigned char *rxtx_buffer, unsigned short length, unsigned char *dest_mac, unsigned char *dest_ip );
void IcmpSendUnreachable(unsigned char *buffer, const unsigned char remoteMac[MAC_ADDRESS_SIZE], const unsigned char remoteIp[IP_V4_ADDRESS_SIZE], unsigned short ipDataLengt);
#endif
