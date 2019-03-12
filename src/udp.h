#ifndef UDP
#define UDP
//********************************************************************************************
//
// File : udp.h implement for User Datagram Protocol
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

#define UDP_HEADER_LEN      8

#define UDP_SRC_PORT_H_P	0x22
#define UDP_SRC_PORT_L_P	0x23
#define UDP_DST_PORT_H_P	0x24
#define UDP_DST_PORT_L_P	0x25
#define UDP_LENGTH_P		0x26
#define UDP_LENGTH_H_P		0x26
#define UDP_LENGTH_L_P		0x27
#define UDP_CHECKSUM_P	    0x28
#define UDP_CHECKSUM_H_P	0x28
#define UDP_CHECKSUM_L_P	0x29
#define UDP_DATA_P			0x2A

typedef struct{
 unsigned short port;
 unsigned short remotePort;
 unsigned char ip[IP_V4_ADDRESS_SIZE];
 unsigned char mac[MAC_ADDRESS_SIZE];
} UdpDatagram;

unsigned short UdpSendDataMac(const unsigned char *mac, const unsigned char *ip, const unsigned short remotePort, const unsigned short port, const unsigned char *data, const unsigned short dataLength);
unsigned short UdpSendData(const unsigned char *ip, const unsigned short remotePort, const unsigned short port, const unsigned char *data, const unsigned short dataLength);
unsigned short UdpSendDataTmpPort(const unsigned char *ip, const unsigned short remotePort, const unsigned char *data, const unsigned short dataLength);
unsigned char UdpReceiveData(const unsigned char *ip, const unsigned short remotePort, const unsigned port, unsigned short timeout, unsigned char **data, unsigned short *dataLength);
void UdpHandleIncomingPacket(unsigned char *buffer, const unsigned short length);
unsigned char UdpOnIncomingDatagram(const UdpDatagram datagram, const unsigned char *data, unsigned short dataLength);
#endif
