//********************************************************************************************
//
// File : udp.c implement for User Datagram Protocol
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
#include <string.h>
#include <stdio.h> //todo vyhodit testovaci kod
#include "enc28j60.h"
#include "ethernet.h"
#include "ip.h"
#include "udp.h"
#include "util.c"
//
//********************************************************************************************
// The User Datagram Protocol offers only a minimal transport service
// -- non-guaranteed datagram delivery
// -- and gives applications direct access to the datagram service of the IP layer.
// UDP is used by applications that do not require the level of service of TCP or
// that wish to use communications services (e.g., multicast or broadcast delivery)
// not available from TCP.
//
// +------------+-----------+-------------+----------+
// + MAC header + IP header +  UDP header + Data ::: +
// +------------+-----------+-------------+----------+
//
// UDP header
//
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +00+01+02+03+04+05+06+07+08+09+10+11+12+13+14+15+16+17+18+19+20+21+22+23+24+25+26+27+28+29+30+31+
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                  Source port                  +               Destination port                +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                  Length                       +               Checksum                        +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                           Data :::                                            +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
//********************************************************************************************
//
// Function : udp_generate_header
// Argument : BYTE *rxtx_buffer is a pointer point to UDP tx buffer
//			  WORD_BYTES dest_port is a destiantion port
//			  WORD_BYTES length is a UDP header and data length
// Return value : None
//
// Description : generate udp header
//
//********************************************************************************************
static void UdpGenerateHeader(unsigned char *buffer, const unsigned short sourcePort, const unsigned short destPort, const unsigned short length){
 // setup source port
 buffer[UDP_SRC_PORT_H_P] = ((unsigned char*)&sourcePort)[1];
 buffer[UDP_SRC_PORT_L_P] = ((unsigned char*)&sourcePort)[0];
 // setup destination port
 buffer[UDP_DST_PORT_H_P] = ((unsigned char*)&destPort)[1];
 buffer[UDP_DST_PORT_L_P] = ((unsigned char*)&destPort)[0];
 // setup udp length
 buffer[UDP_LENGTH_H_P] = ((unsigned char*)&length)[1];
 buffer[UDP_LENGTH_L_P] = ((unsigned char*)&length)[0];
 // setup udp checksum
 buffer[UDP_CHECKSUM_H_P] = 0;
 buffer[UDP_CHECKSUM_L_P] = 0;
 // length+8 for source/destination IP address length (8-bytes)
 CharsPutShort(buffer + UDP_CHECKSUM_P, software_checksum(buffer + IP_SRC_IP_P, length + 8, length + IP_PROTO_UDP_V));
}

//********************************************************************************************
//
// Function : UdpSend
// Description : send upd data into network
//
//********************************************************************************************
unsigned char UdpSend(unsigned char* buffer, const unsigned char* mac, const unsigned char *ip, const unsigned short port, const unsigned short remotePort, const unsigned char *data, const unsigned short dataLength){
 if(dataLength + ETH_HEADER_LEN + IP_HEADER_LEN + UDP_HEADER_LEN + dataLength > MAX_TX_BUFFER){
  return 0;
 }
 eth_generate_header(buffer, (WORD_BYTES){ETH_TYPE_IP_V}, mac);
 ip_generate_header(buffer, (WORD_BYTES){IP_HEADER_LEN + UDP_HEADER_LEN+dataLength}, IP_PROTO_UDP_V, ip);
 memcpy(buffer + UDP_DATA_P, data, dataLength);
 UdpGenerateHeader(buffer, port, remotePort, UDP_HEADER_LEN + dataLength);
 // send packet to ethernet media
 enc28j60_packet_send(buffer, ETH_HEADER_LEN + IP_HEADER_LEN + UDP_HEADER_LEN + dataLength);
 return 1;
}

// todo vyhodit testovaci kod
//********************************************************************************************
//
// Function : UdpHandleIncomingPacket
// Description : hand incoming udp packet form network
//
//********************************************************************************************
void UdpHandleIncomingPacket(unsigned char *buffer, unsigned short length, const unsigned char srcMac[MAC_ADDRESS_SIZE], const unsigned char srcIp[IP_V4_ADDRESS_SIZE]){
 length = CharsToShort(buffer + UDP_LENGTH_P) - UDP_HEADER_LEN;
 char out[100];
 sprintf(out, "UDP Delka dat %u\n", length);
 UARTWriteChars(out);
 sprintf(out, "UDP Port %u remote port %u\n", CharsToShort(buffer + UDP_DST_PORT_H_P), CharsToShort(buffer + UDP_SRC_PORT_H_P));
 UARTWriteChars(out);
 UARTWriteChars("UDP Prichozi data '");
 UARTWriteCharsLength(buffer + UDP_DATA_P, length);
 UARTWriteChars("'\n");
 UdpSend(buffer, srcMac, srcIp, CharsToShort(buffer + UDP_DST_PORT_H_P), CharsToShort(buffer + UDP_SRC_PORT_H_P), buffer + UDP_DATA_P, length);
 //todo pridani callbacku na prichozi packaket
}
