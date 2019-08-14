#ifndef TCP
#define TCP
//********************************************************************************************
//
// File : tcp.h implement for Transmission Control Protocol
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
#define TCP_HEADER_LEN		20
#define TCP_OPTION_LEN		4

#define TCP_INVALID_CONNECTION_ID 0xff

#define TCP_STATE_NO_CONNECTION 0
#define TCP_STATE_NEW 1
#define TCP_STATE_SYN_RECEIVED 2
#define TCP_STATE_ESTABLISHED 3
#define TCP_STATE_DYEING 4

#define TCP_MAX_SEGMENT_SIZE (MAX_RX_BUFFER - ETH_HEADER_LEN - IP_HEADER_LEN - TCP_HEADER_LEN)

#define TCP_FLAG_FIN_V		0x01
#define TCP_FLAG_SYN_V		0x02
#define TCP_FLAG_RST_V		0x04
#define TCP_FLAG_PSH_V		0x08
#define TCP_FLAG_ACK_V		0x10
#define TCP_FLAG_URG_V		0x20
#define TCP_FLAG_ECE_V		0x40
#define TCP_FLAG_CWR_V		0x80

#define TCP_OPTION_END_LIST_KIND        0
#define TCP_OPTION_NO_OPERATION_KIND    1
#define TCP_OPTION_MAX_SEGMET_SIZE_KIND 2

#define TCP_SRC_PORT_P 	    0x22
#define TCP_SRC_PORT_H_P 	0x22
#define TCP_SRC_PORT_L_P 	0x23
#define TCP_DST_PORT_P 	    0x24
#define TCP_DST_PORT_H_P 	0x24
#define TCP_DST_PORT_L_P 	0x25
#define TCP_SEQ_P  			0x26	// the tcp seq number is 4 bytes 0x26-0x29
#define TCP_SEQACK_P  		0x2A	// 4 bytes
#define TCP_HEADER_LEN_P 	0x2E
#define TCP_FLAGS_P 		0x2F
#define TCP_WINDOWSIZE_H_P	0x30	// 2 bytes
#define TCP_WINDOWSIZE_L_P	0x31
#define TCP_CHECKSUM_H_P 	0x32
#define TCP_CHECKSUM_L_P 	0x33
#define TCP_URGENT_PTR_H_P 	0x34	// 2 bytes
#define TCP_URGENT_PTR_L_P 	0x35
#define TCP_OPTIONS_P 		0x36
#define TCP_DATA_P			0x36

#ifndef TCP_ON_NEW_CONNETION_CALLBACK
#define TCP_ON_NEW_CONNETION_CALLBACK TcpOnNewConnection
#endif
#ifndef TCP_ON_CONNECT_CALLBACK
#define TCP_ON_CONNECT_CALLBACK TcpOnConnect
#endif
#ifndef TCP_ON_INCOMING_DATA_CALLBACK
#define TCP_ON_INCOMING_DATA_CALLBACK TcpOnIncomingData
#endif
#ifndef TCP_ON_DISCONNECT_CALLBACK
#define TCP_ON_DISCONNECT_CALLBACK TcpOnDisconnect
#endif

typedef struct{
 unsigned long sendSequence;
 unsigned long expectedSequence;
 unsigned short port;
 unsigned short remotePort;
 unsigned short maxSegmetSize;
 unsigned char state;
 unsigned char ip[IP_V4_ADDRESS_SIZE];
 unsigned char mac[MAC_ADDRESS_SIZE];
} TcpConnection;

//********************************************************************************************
//
// Prototype function
//
//********************************************************************************************
void TcpInit();
const TcpConnection *TcpGetConnection(const unsigned char connectionId);
unsigned char TcpConnect(const unsigned char ip[IP_V4_ADDRESS_SIZE], const unsigned short remotePort, const unsigned short timeout);
unsigned char TcpSendData(const unsigned char connectionId, const unsigned short timeout, const unsigned char *data, unsigned short dataLength);
unsigned char TcpReceiveData(const unsigned char connectionId, const unsigned short timeout, unsigned char **data, unsigned short *dataLength);
unsigned char TcpDisconnect(const unsigned char connectionId, unsigned short timeout);
void TcpHandleIncomingPacket(unsigned char *buffer, const unsigned short length);
unsigned char TCP_ON_NEW_CONNETION_CALLBACK(const unsigned char connectionId);
void TCP_ON_CONNECT_CALLBACK(const unsigned char connectionId);
void TCP_ON_INCOMING_DATA_CALLBACK(const unsigned char connectionId, const unsigned char *data, unsigned short dataLength);
void TCP_ON_DISCONNECT_CALLBACK(const unsigned char connectionId);
#endif
