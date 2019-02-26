//********************************************************************************************
//
// File : icmp.c implement for Internet Control Message Protocol
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
#include "enc28j60.h"
#include "ethernet.h"
#include "ip.h"
#include "icmp.h"
#include "util.c"
//********************************************************************************************
//
// The Internet Control Message Protocol (ICMP) is one of the core protocols of the
// Internet protocol suite. It is chiefly used by networked computers'
// operating systems to send error messages---indicating, for instance,
// that a requested service is not available or that a host or router could not be reached.
//
// ICMP differs in purpose from TCP and UDP in that it is usually not used directly
// by user network applications. One exception is the ping tool,
// which sends ICMP Echo Request messages (and receives Echo Response messages)
// to determine whether a host is reachable and how long packets take to get to and from that host.
//
// +------------+-----------+-------------+----------+
// + MAC header + IP header + ICMP header + Data ::: +
// +------------+-----------+-------------+----------+
//
// ICMP header
//
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +00+01+02+03+04+05+06+07+08+09+10+11+12+13+14+15+16+17+18+19+20+21+22+23+24+25+26+27+28+29+30+31+
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +         Type          +          Code         +               ICMP header checksum            +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                            Data :::                                           +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
//********************************************************************************************

//*******************************************************************************************
//
// Function : icmp_send_reply
// Description : Send ARP reply packet from ARP request packet
//
//*******************************************************************************************
static void IcmpGeneratePacket(unsigned char *buffer, unsigned char icmpType, unsigned char icmpCode, unsigned char dataLength){
 buffer[ICMP_TYPE_P] = icmpType;
 buffer[ICMP_CODE_P] = icmpCode;
 // clear icmp checksum
 buffer[ICMP_CHECKSUM_H_P] = 0;
 buffer[ICMP_CHECKSUM_L_P] = 0;
 // calculate new checksum.
 // ICMP checksum calculation begin at ICMP type to ICMP data.
 // Before calculate new checksum the checksum field must be zero.
 unsigned short test = software_checksum(buffer + ICMP_TYPE_P, ICMP_HEADER_LEN + dataLength, 0);
 CharsPutShort(buffer + ICMP_CHECKSUM_H_P, test);
}

/*
//*******************************************************************************************
//
// Function : icmp_send_request
// Description : Send ARP request packet to destination.
//
//*******************************************************************************************
void icmp_send_request ( BYTE *rxtx_buffer, unsigned char length, BYTE *dest_mac, BYTE *dest_ip )
{
	// set ethernet header
	eth_generate_header ( rxtx_buffer, (WORD_BYTES){ETH_TYPE_IP_V}, dest_mac );

	// generate ip header and checksum
	ip_generate_header (	rxtx_buffer, (WORD_BYTES){sizeof(IP_HEADER) + sizeof(ICMP_HEADER)}, IP_PROTO_ICMP_V, dest_ip );

	// generate icmp packet and checksum
	rxtx_buffer[ ICMP_TYPE_P ] = ICMP_TYPE_ECHOREQUEST_V;
	rxtx_buffer[ ICMP_CODE_P ] = 0;
	rxtx_buffer[ ICMP_IDENTIFIER_H_P ] = icmp_id;
	rxtx_buffer[ ICMP_IDENTIFIER_L_P ] = 0;
	rxtx_buffer[ ICMP_SEQUENCE_H_P ] = icmp_seq;
	rxtx_buffer[ ICMP_SEQUENCE_L_P ] = 0;
	icmp_id++;
	icmp_seq++;
	icmp_generate_packet ( rxtx_buffer, length );

	// send packet to ethernet media
	enc28j60_packet_send ( rxtx_buffer, sizeof(ETH_HEADER) + sizeof(IP_HEADER) + sizeof(ICMP_HEADER) + length );
}
*/

//*******************************************************************************************
//
// Function : icmp_send_reply
// Description : Send PING reply packet to destination.
//
//*******************************************************************************************
unsigned char icmp_send_reply ( unsigned char *rxtx_buffer, unsigned short length, unsigned char *dest_mac, unsigned char *destIp){
 // check protocol is icmp or not?
 if ( rxtx_buffer [ IP_PROTO_P ] != IP_PROTO_ICMP_V ){
  return 0;
 }
 // check icmp packet type is echo request or not?
 if ( rxtx_buffer [ ICMP_TYPE_P ] != ICMP_TYPE_ECHOREQUEST_V ){
  return 0;
 }
 eth_generate_header(rxtx_buffer, ETH_TYPE_IP_V, dest_mac);
 ip_generate_header(rxtx_buffer, length - ETH_HEADER_LEN, IP_PROTO_ICMP_V, destIp);
 IcmpGeneratePacket(rxtx_buffer, ICMP_TYPE_ECHOREPLY_V, 0, length - ICMP_DATA_P);
 // send packet to ethernet media
 enc28j60_packet_send(rxtx_buffer, length);
 return 1;
}

//*******************************************************************************************
//
// Function : IcmpSendUnreachable
// Description : Send icmp response for any unreachable service on ip protocol
//
//*******************************************************************************************
void IcmpSendUnreachable(unsigned char *buffer, const unsigned char remoteMac[MAC_ADDRESS_SIZE], const unsigned char remoteIp[IP_V4_ADDRESS_SIZE], unsigned short ipTotalLength){
 if(ipTotalLength > 8 + IP_HEADER_LEN){
  ipTotalLength = 8 + IP_HEADER_LEN;
 }
 memcpy(buffer + ICMP_DATA_P, buffer + IP_P, ipTotalLength);
 IcmpGeneratePacket(buffer, ICMP_TYPE_UNREACHABLE_V, ICMP_CODE_UNREACHABLE_HOST_SERVICE_V, ipTotalLength);
 eth_generate_header(buffer, ETH_TYPE_IP_V, remoteMac);
 ip_generate_header(buffer, IP_HEADER_LEN + ICMP_HEADER_LEN + ipTotalLength, IP_PROTO_ICMP_V, remoteIp);
 enc28j60_packet_send(buffer, ETH_HEADER_LEN + IP_HEADER_LEN + ICMP_HEADER_LEN + ipTotalLength);
}

//*******************************************************************************************
//
// Function : icmp_ping_server
// Description : Send ARP reply packet to destination.
//
//*******************************************************************************************
/*BYTE icmp_ping ( BYTE *rxtx_buffer, BYTE *dest_mac, BYTE *dest_ip )
{
	BYTE i;
	WORD dlength;

	// destination ip was not found on network.
	if ( arp_who_is ( rxtx_buffer, dest_mac, dest_ip ) == 0 )
		return 0;

	// send icmp request packet (ping) to server
	icmp_send_request ( rxtx_buffer, dest_mac, dest_ip );

	for ( i=0; i<10; i++ )
	{
		_delay_ms( 10 );
		dlength = enc28j60_packet_receive( rxtx_buffer, MAX_RXTX_BUFFER );

		if ( dlength )
		{
			// check protocol is icmp or not?
			if ( rxtx_buffer [ IP_PROTO_P ] != IP_PROTO_ICMP_V )
				continue;

			// check icmp packet type is echo reply or not?
			if ( rxtx_buffer [ ICMP_TYPE_P ] != ICMP_TYPE_ECHOREPLY_V )
				continue;

			return 1;
		}
	}

	// time out
	return 0;
}*/
