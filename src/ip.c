//********************************************************************************************
//
// File : ip.c implement for Internet Protocol
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
#include "ip.h"
#include "ethernet.h"
//********************************************************************************************
//
// +------------+-----------+----------+
// + MAC header + IP header + Data ::: +
// +------------+-----------+----------+
//
// IP Header
//
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +00+01+02+03+04+05+06+07+08+09+10+11+12+13+14+15+16+17+18+19+20+21+22+23+24+25+26+27+28+29+30+31+
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// + Version   +   IHL     +             TOS       +                Total length                   +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                  Identification               +  Flags +          Fragment offset             +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +          TTL          +       Protocol        +                Header checksum                +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                      Source IP address                                        +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                    Destination IP address                                     +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                    Options and padding :::                                    +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
//********************************************************************************************
static unsigned short ip_identfier=1;
//********************************************************************************************
//
// Function : ip_generate_packet
// Description : generate all ip header
//
//********************************************************************************************
void ip_generate_header(unsigned char *buffer, unsigned short totalLength, unsigned char protocol, const unsigned char destIp[IP_V4_ADDRESS_SIZE]){
	// set ipv4 and header length
	buffer[ IP_P ] = IP_V4_V | IP_HEADER_LENGTH_V;

	// set TOS to default 0x00
	buffer[ IP_TOS_P ] = 0x00;

	// set total length
    CharsPutShort(buffer + IP_TOTLEN_H_P, totalLength);

	// set packet identification
	buffer [ IP_ID_H_P ] = High(ip_identfier);
	buffer [ IP_ID_L_P ] = Low(ip_identfier);
	ip_identfier++;

	// set fragment flags
	buffer [ IP_FLAGS_H_P ] = 0x00;
	buffer [ IP_FLAGS_L_P ] = 0x00;

	// set Time To Live
	buffer [ IP_TTL_P ] = 128;

	// set ip packettype to tcp/udp/icmp...
	buffer [ IP_PROTO_P ] = protocol;

	// set source and destination ip address
    memcpy(buffer + IP_DST_IP_P, destIp, IP_V4_ADDRESS_SIZE);
    memcpy(buffer + IP_SRC_IP_P, avrIp, IP_V4_ADDRESS_SIZE);

	// clear the 2 byte checksum
	buffer[ IP_CHECKSUM_H_P ] = 0;
	buffer[ IP_CHECKSUM_L_P ] = 0;

	// fill checksum value
	// calculate the checksum:
	CharsPutShort(buffer + IP_CHECKSUM_H_P, software_checksum (buffer + IP_P, IP_HEADER_LEN, 0));
}
//********************************************************************************************
//
// Function : ip_packet_is_ip
// Description : Check incoming packet
//
//********************************************************************************************
unsigned char ip_packet_is_ip(unsigned char *rxtx_buffer){
 if(rxtx_buffer[ ETH_TYPE_H_P ] != ETH_TYPE_IP_H_V || rxtx_buffer[ ETH_TYPE_L_P ] != ETH_TYPE_IP_L_V){
  return 0;
 }
 // if ip packet not send to avr
 if(memcmp(avrIp, rxtx_buffer + IP_DST_IP_P, IP_V4_ADDRESS_SIZE) != 0){
  return 0;
 }
 return 1;
}
