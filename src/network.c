#include <avr/io.h>
#include <util/delay.h>
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
static unsigned char buffer[NET_BUFFER_SIZE];

void NetInit() {
 enc28j60_init();
 ArpInit();
 #ifdef TCP
 TcpInit();
 #endif
 #ifdef HTTP
 HttpInit();
 #endif
}

unsigned char *NetGetBuffer(){
 return buffer;
}

void NetHandleNetwork(){
 unsigned short length;
 length = enc28j60_packet_receive(buffer, NET_BUFFER_SIZE);
 if(length == 0){
  return;
 }
 NetHandleIncomingPacket(length);
}

void NetHandleIncomingPacket(unsigned short length){
 {
  unsigned char srcMac[MAC_ADDRESS_SIZE];
  memcpy(srcMac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE);
  if(ArpPacketIsArp(buffer, ARP_OPCODE_REQUEST_V)){
   ArpSendReply(buffer, srcMac);
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
 if(buffer[IP_PROTO_P] == IP_PROTO_TCP_V){
  TcpHandleIncomingPacket(buffer, length);
  return;
 }
 #endif
}
