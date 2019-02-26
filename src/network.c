#include <string.h>
#include "util.c"
#include "enc28j60.c"
#include "ethernet.c"
#include "arp.c"
#include "ip.c"
#include "network.h"

#if defined(TCP) || defined(UDP)
unsigned short connectPortRotaiting = NET_MIN_DINAMIC_PORT;
#endif

void NetInit() {
 enc28j60_init();
 ArpInit();
 #ifdef TCP
 TcpInit();
 #endif
}

void NetHandleNetwork(){
 unsigned short length;
 unsigned char buffer[NET_BUFFER_SIZE];// todo skovat buffer do klihovny
 length = enc28j60_packet_receive(buffer, NET_BUFFER_SIZE);
 if(length == 0){
  return;
 }
 NetHandleIncomingPacket(buffer, length);
}

void NetHandleIncomingPacket(unsigned char *buffer, unsigned short length){
 {
  unsigned char srcMac[MAC_ADDRESS_SIZE];
  memcpy(srcMac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE);
  if(arp_packet_is_arp(buffer, ARP_OPCODE_REQUEST_V)){
   arp_send_reply(buffer, srcMac);
   return;
  }
  if(!ip_packet_is_ip(buffer)){
   return;
  }
  unsigned char srcIp[IP_V4_ADDRESS_SIZE];
  memcpy(srcIp, buffer + IP_SRC_IP_P, IP_V4_ADDRESS_SIZE);
  #ifdef ICMP
  if(icmp_send_reply(buffer, length, srcMac, srcIp)){
   return;
  }
  #endif
 }
 #ifdef UDP
 if(buffer[IP_PROTO_P] == IP_PROTO_UDP_V){
  UdpHandleIncomingPacket(buffer, length);
  return;
 }
 #endif
 #ifdef TCP
 if(TcpIsTcp(buffer)){
  TcpHandleIncomingPacket(buffer, length);
  return;
 }
 #endif
}
