#ifndef ARP
#define ARP
//********************************************************************************************
//
// File : arp.h implement for Address Resolution Protocol
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
#define ARP_V4_PACKET_LEN			28

#define ARP_OPCODE_REQUEST_V	0x0001
#define ARP_OPCODE_REQUEST_H_V	0x00
#define ARP_OPCODE_REQUEST_L_V	0x01
#define ARP_OPCODE_REPLY_V		0x0002
#define ARP_OPCODE_REPLY_H_V	0x00
#define ARP_OPCODE_REPLY_L_V	0x02

#define ARP_HARDWARE_TYPE_H_V	0x00
#define ARP_HARDWARE_TYPE_L_V	0x01
#define ARP_PROTOCOL_H_V		0x08
#define ARP_PROTOCOL_L_V		0x00
#define ARP_HARDWARE_SIZE_V		0x06
#define ARP_PROTOCOL_SIZE_V		0x04

#define ARP_HARDWARE_TYPE_H_P	0x0E
#define ARP_HARDWARE_TYPE_L_P	0x0F
#define ARP_PROTOCOL_H_P		0x10
#define ARP_PROTOCOL_L_P		0x11
#define ARP_HARDWARE_SIZE_P		0x12
#define ARP_PROTOCOL_SIZE_P		0x13
#define ARP_OPCODE_H_P			0x14
#define ARP_OPCODE_L_P			0x15
#define ARP_SRC_MAC_P			0x16
#define ARP_SRC_IP_P			0x1C
#define ARP_DST_MAC_P			0x20
#define ARP_DST_IP_P			0x26

#ifndef NET_ARP_CACHE_SIZE
#define NET_ARP_CACHE_SIZE     5
#endif

#define ARP_CACHE_USE_COUNT_P   (MAC_ADDRESS_SIZE + IP_V4_ADDRESS_SIZE)
#define ARP_CACHE_MAC_P         0
#define ARP_CACHE_IP_P          (MAC_ADDRESS_SIZE)

//********************************************************************************************
//
// Prototype function
//
//********************************************************************************************
void ArpInit();
void ArpSendReply ( unsigned char *rxtx_buffer, unsigned char *dest_mac );
unsigned char ArpPacketIsArp ( unsigned char *rxtx_buffer, unsigned short opcode );
unsigned char ArpWhoIs(unsigned char *buffer, const unsigned char destIp[IP_V4_ADDRESS_SIZE], unsigned char destMac[MAC_ADDRESS_SIZE]);
#endif
