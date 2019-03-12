//********************************************************************************************
//
// File : arp.c implement for Address Resolution Protocol
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
#include "arp.h"
#include "ethernet.h"
#include "network.h"
#include <string.h>
//********************************************************************************************
//
// Address Resolution Protocol (ARP) is the method for finding a host's hardware address
// when only its network layer address is known.
// Due to the overwhelming prevalence of IPv4 and Ethernet,
// ARP is primarily used to translate IP addresses to Ethernet MAC addresses.
// It is also used for IP over other LAN technologies,
// such as Token Ring, FDDI, or IEEE 802.11, and for IP over ATM.
//
// ARP is used in four cases of two hosts communicating:
//
//   1. When two hosts are on the same network and one desires to send a packet to the other
//   2. When two hosts are on different networks and must use a gateway/router to reach the other host
//   3. When a router needs to forward a packet for one host through another router
//   4. When a router needs to forward a packet from one host to the destination host on the same network
//
// +------------+------------+-----------+
// + MAC header + ARP header +	Data ::: +
// +------------+------------+-----------+
//
// ARP header
//
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +00+01+02+03+04+05+06+07+08+09+10+11+12+13+14+15+16+17+18+19+20+21+22+23+24+25+26+27+28+29+30+31+
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +             Hardware type 	                   +                    Protocol type              +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// + HardwareAddressLength + ProtocolAddressLength +	                 Opcode                    +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                   Source hardware address :::                                 +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                   Source protocol address :::                                 +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                Destination hardware address :::                               +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                Destination protocol address :::                               +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                          Data :::                                             +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
//********************************************************************************************

const unsigned char avrIp[IP_V4_ADDRESS_SIZE] = {NET_IP};
#if NET_ARP_CACHE_SIZE > 0
static unsigned char arpCache[NET_ARP_CACHE_SIZE][MAC_ADDRESS_SIZE + IP_V4_ADDRESS_SIZE + 1];
#endif


//********************************************************************************************
//
// Function : ArpInit
// Description : initialization of memory
//
//********************************************************************************************
void ArpInit(){
 #if NET_ARP_CACHE_SIZE > 0
 unsigned char i;
 for(i=0; i<NET_ARP_CACHE_SIZE; i++){
  arpCache[i][ARP_CACHE_USE_COUNT_P] = 0;
 }
 #endif
}

//********************************************************************************************
//
// Function : arp_generate_packet
// Description : generate arp packet
//
//********************************************************************************************
static void arp_generate_packet( unsigned char *rxtx_buffer, unsigned char *dest_mac, const unsigned char *dest_ip ){
 // setup hardware type to ethernet 0x0001
 rxtx_buffer[ ARP_HARDWARE_TYPE_H_P ] = ARP_HARDWARE_TYPE_H_V;
 rxtx_buffer[ ARP_HARDWARE_TYPE_L_P ] = ARP_HARDWARE_TYPE_L_V;
 // setup protocol type to ip 0x0800
 rxtx_buffer[ ARP_PROTOCOL_H_P ] = ARP_PROTOCOL_H_V;
 rxtx_buffer[ ARP_PROTOCOL_L_P ] = ARP_PROTOCOL_L_V;
 // setup hardware length to 0x06
 rxtx_buffer[ ARP_HARDWARE_SIZE_P ] = ARP_HARDWARE_SIZE_V;
 // setup protocol length to 0x04
 rxtx_buffer[ ARP_PROTOCOL_SIZE_P ] = ARP_PROTOCOL_SIZE_V;
 // setup arp destination and source mac address
 memcpy(rxtx_buffer + ARP_DST_MAC_P, dest_mac, MAC_ADDRESS_SIZE);
 memcpy(rxtx_buffer + ARP_SRC_MAC_P, avr_mac, MAC_ADDRESS_SIZE);
 // setup arp destination and source ip address
 memcpy(rxtx_buffer + ARP_DST_IP_P, dest_ip, IP_V4_ADDRESS_SIZE);
 memcpy(rxtx_buffer + ARP_SRC_IP_P, avrIp, IP_V4_ADDRESS_SIZE);
}

//********************************************************************************************
//
// Function : arp_send_request
// Description : send arp request packet (who is?) to network.
//
//********************************************************************************************
static void arp_send_request(unsigned char *rxtx_buffer, const unsigned char *destIp){
 unsigned char i, destMac[MAC_ADDRESS_SIZE];
 // generate ethernet header
 for ( i=0; i<MAC_ADDRESS_SIZE; i++){
  destMac[i] = 0xff;
 }
 eth_generate_header(rxtx_buffer, ETH_TYPE_ARP_V, destMac);
 // generate arp packet
 for ( i=0; i<MAC_ADDRESS_SIZE; i++){
  destMac[i] = 0x00;
 }
 // set arp opcode is request
 rxtx_buffer[ ARP_OPCODE_H_P ] = ARP_OPCODE_REQUEST_H_V;
 rxtx_buffer[ ARP_OPCODE_L_P ] = ARP_OPCODE_REQUEST_L_V;
 arp_generate_packet(rxtx_buffer, destMac, destIp);
 // send arp packet to network
 enc28j60_packet_send(rxtx_buffer, ETH_HEADER_LEN + ARP_V4_PACKET_LEN);
}

//*******************************************************************************************
//
// Function : ArpPacketIsArp
// Description : check received packet, that packet is match with arp and avr ip or not?
//
//*******************************************************************************************
unsigned char ArpPacketIsArp ( unsigned char *rxtx_buffer, unsigned short opcode ){
 // if packet type is not arp packet exit from function
 if( rxtx_buffer[ ETH_TYPE_H_P ] != ETH_TYPE_ARP_H_V || rxtx_buffer[ ETH_TYPE_L_P ] != ETH_TYPE_ARP_L_V){
  return 0;
 }
 // check arp request opcode
 if ( rxtx_buffer[ ARP_OPCODE_H_P ] != ((unsigned char *)&opcode)[1] || rxtx_buffer[ ARP_OPCODE_L_P ] != ((unsigned char *)&opcode)[0]){
  return 0;
 }
 // if destination ip address in arp packet not match with avr ip address
 return memcmp(rxtx_buffer + ARP_DST_IP_P, avrIp, IP_V4_ADDRESS_SIZE) == 0;
}

//*******************************************************************************************
//
// Function : ArpSendReply
// Description : Send reply if recieved packet is ARP and IP address is match with avr_ip
//
//*******************************************************************************************
void ArpSendReply ( unsigned char *rxtx_buffer, unsigned char *dest_mac ){
	// generate ethernet header
	eth_generate_header(rxtx_buffer, ETH_TYPE_ARP_V, dest_mac);

	// change packet type to echo reply
	rxtx_buffer[ ARP_OPCODE_H_P ] = ARP_OPCODE_REPLY_H_V;
	rxtx_buffer[ ARP_OPCODE_L_P ] = ARP_OPCODE_REPLY_L_V;
	arp_generate_packet ( rxtx_buffer, dest_mac, &rxtx_buffer[ ARP_SRC_IP_P ] );

	// send arp packet
	enc28j60_packet_send ( rxtx_buffer, ETH_HEADER_LEN + ARP_V4_PACKET_LEN);
}

//*******************************************************************************************
//
// Function : arpWhoIs
// Description : send arp request to destination ip, and save destination mac to dest_mac.
// call this function to find the destination mac address before send other packet.
//
//*******************************************************************************************
unsigned char ArpWhoIs(unsigned char *buffer, const unsigned char destIp[IP_V4_ADDRESS_SIZE], unsigned char destMac[MAC_ADDRESS_SIZE]){
 unsigned short dlength;
 unsigned char i, found = 0, minUseCacheId = 0;
 #if NET_ARP_CACHE_SIZE > 0
 // try found mac address in cache
 for(i=0; i<NET_ARP_CACHE_SIZE; i++){
  if(!found && arpCache[i][ARP_CACHE_USE_COUNT_P] && memcmp(arpCache[i] + ARP_CACHE_IP_P, destIp, IP_V4_ADDRESS_SIZE) == 0){
   memcpy(destMac, arpCache[i] + ARP_CACHE_MAC_P, MAC_ADDRESS_SIZE);
   arpCache[i][ARP_CACHE_USE_COUNT_P] = (255 - arpCache[i][ARP_CACHE_USE_COUNT_P] >= 2) ? arpCache[i][ARP_CACHE_USE_COUNT_P] + 2 : 255;
   found = 1;
   continue;
  }
  if(arpCache[i][ARP_CACHE_USE_COUNT_P]){
   arpCache[i][ARP_CACHE_USE_COUNT_P]--;
  }
  if(arpCache[minUseCacheId][ARP_CACHE_USE_COUNT_P] < arpCache[i][ARP_CACHE_USE_COUNT_P]){
   minUseCacheId = i;
  }
 }
 if(found){
  return 1;
 }
 #endif
 arp_send_request(buffer, destIp);
 dlength = EthWaitPacket(buffer, ETH_TYPE_ARP_V, 100);
 if(dlength != 0){
  if(ArpPacketIsArp(buffer, ARP_OPCODE_REPLY_V)){
   // copy destination mac address from arp reply packet to destination mac address
   memcpy(destMac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE);
   #if NET_ARP_CACHE_SIZE > 0
   // save destination mac into cache
   memcpy(arpCache[minUseCacheId] + ARP_CACHE_MAC_P, destMac, MAC_ADDRESS_SIZE);
   memcpy(arpCache[minUseCacheId] + ARP_CACHE_IP_P, destIp, IP_V4_ADDRESS_SIZE);
   if(!arpCache[minUseCacheId][ARP_CACHE_USE_COUNT_P]){
    arpCache[minUseCacheId][ARP_CACHE_USE_COUNT_P] = 2;
   }
   #endif
   return 1;
  }
  NetHandleIncomingPacket(dlength);
 }
 return 0;
}
