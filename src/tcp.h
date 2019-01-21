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

#ifndef TCP_MAX_CONNECTIONS
#define TCP_MAX_CONNECTIONS 2
#endif

#define TCP_INVALID_CONNECTION_ID 0xff

#define TCP_STATE_NO_CONNECTION 0
#define TCP_STATE_NEW 1
#define TCP_STATE_SYN_RECEIVED 2
#define TCP_STATE_ESTABLISHED 3
#define TCP_STATE_DYEING 4

#define TCP_FLAG_FIN_V		0x01
#define TCP_FLAG_SYN_V		0x02
#define TCP_FLAG_RST_V		0x04
#define TCP_FLAG_PSH_V		0x08
#define TCP_FLAG_ACK_V		0x10
#define TCP_FLAG_URG_V		0x20
#define TCP_FLAG_ECE_V		0x40
#define TCP_FLAG_CWR_V		0x80

#define TCP_SRC_PORT_H_P 	0x22
#define TCP_SRC_PORT_L_P 	0x23
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

typedef struct TcpConnection{
 unsigned long sendSequence;
 unsigned long expectedSequence;
 unsigned short port;
 unsigned short remotePort;
 unsigned char ip[IP_V4_ADDRESS_SIZE];
 unsigned char mac[MAC_ADDRESS_SIZE];
 unsigned char state;
} TcpConnection;

//********************************************************************************************
//
// Prototype function
//
//********************************************************************************************
void TcpInit();
WORD tcp_get_dlength( BYTE *rxtx_buffer );
BYTE tcp_get_hlength( BYTE *rxtx_buffer );
WORD tcp_puts_data( BYTE *rxtx_buffer, BYTE *data, WORD offset );
unsigned char tcp_is_tcp(unsigned char *rxtx_buffer, unsigned short dest_port, unsigned short* src_port);
unsigned char TcpSendAck(unsigned char *buffer, unsigned long sequence, unsigned short dataLength, TcpConnection *conenction);// todo je to potreba mit z venci
void tcp_get_sequence(unsigned char *buffer, unsigned long *seq, unsigned long *ack);
void TcpConnect(unsigned char *buffer, TcpConnection *connection, const unsigned char *desIp, const unsigned short destPort, const unsigned short srcPort, const unsigned short timeout);
unsigned char TcpGetEmptyConenctionId();
unsigned char TcpSendData(unsigned char *buffer, TcpConnection *conection, unsigned short timeout, unsigned char *data, unsigned short dataLength);
void tcp_handle_incoming_packet(unsigned char buffer[], unsigned short length, unsigned char srcMac[], unsigned char srcIp[], unsigned short srcPort, unsigned short destPort);
#endif
