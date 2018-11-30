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
#include "udp.h"
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
void udp_generate_header ( BYTE *rxtx_buffer, unsigned short source_port, WORD_BYTES dest_port, WORD_BYTES length )
{
	WORD_BYTES ck;

	// setup source port
	rxtx_buffer[UDP_SRC_PORT_H_P] = *((unsigned char *)(&source_port));
	rxtx_buffer[UDP_SRC_PORT_L_P] = *(((unsigned char *)(&source_port))+1);

	// setup destination port
	rxtx_buffer[UDP_DST_PORT_H_P] = dest_port.byte.high;
	rxtx_buffer[UDP_DST_PORT_L_P] = dest_port.byte.low;

	// setup udp length
	rxtx_buffer[UDP_LENGTH_H_P] = length.byte.high;
	rxtx_buffer[UDP_LENGTH_L_P] = length.byte.low;

	// setup udp checksum
	rxtx_buffer[UDP_CHECKSUM_H_P] = 0;
	rxtx_buffer[UDP_CHECKSUM_L_P] = 0;
	// length+8 for source/destination IP address length (8-bytes)
	ck.word = software_checksum ( (BYTE*)&rxtx_buffer[IP_SRC_IP_P], length.word+8, length.word+IP_PROTO_UDP_V);
	rxtx_buffer[UDP_CHECKSUM_H_P] = ck.byte.high;
	rxtx_buffer[UDP_CHECKSUM_L_P] = ck.byte.low;
}
//********************************************************************************************
//
// Function : udp_puts_data
// Description : puts data from RAM to UDP tx buffer
//
//********************************************************************************************
WORD udp_puts_data ( BYTE *rxtx_buffer, BYTE *data, WORD offset, unsigned short length ){
 unsigned short i;
 for(i=0; i<length; i++){
  rxtx_buffer[ UDP_DATA_P + offset ] = data[i];
	offset++;
 }
 return offset;
}

BYTE udp_is_udp ( BYTE *rxtx_buffer, unsigned short dest_port, unsigned short *source_port )
{
	// check UDP packet
	if ( rxtx_buffer[IP_PROTO_P] != IP_PROTO_UDP_V ){
		return 0;
	}
	
	// check destination port
	if(rxtx_buffer[UDP_DST_PORT_H_P] != ((unsigned char*)&dest_port)[1] || rxtx_buffer[ UDP_DST_PORT_L_P ] != ((unsigned char*)&dest_port)[0]){
   return 0;
  }

  ((unsigned char *)source_port)[1] = rxtx_buffer[UDP_SRC_PORT_H_P];
  ((unsigned char *)source_port)[0] = rxtx_buffer[UDP_SRC_PORT_L_P];

	return 1;
}

void udp_send(unsigned char* rxtx_buffer, unsigned short source_port, unsigned char* dest_mac, unsigned char *dest_ip, unsigned short dest_port, unsigned short data_length){
 // set ethernet header
 eth_generate_header (rxtx_buffer, (WORD_BYTES){ETH_TYPE_IP_V}, dest_mac );
 
 // generate ip header and checksum
 ip_generate_header (rxtx_buffer, (WORD_BYTES){sizeof(IP_HEADER)+sizeof(UDP_HEADER)+data_length}, IP_PROTO_UDP_V, dest_ip );
 
 // generate UDP header
 udp_generate_header (rxtx_buffer, source_port, (WORD_BYTES){dest_port}, (WORD_BYTES){sizeof(UDP_HEADER)+data_length});

 // send packet to ethernet media
 enc28j60_packet_send ( rxtx_buffer, sizeof(ETH_HEADER)+sizeof(IP_HEADER)+sizeof(UDP_HEADER)+data_length );
}
