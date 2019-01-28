#ifndef NET
#define NET

#include <string.h>
#include "arp.h"
#include "icmp.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

void NetHandleIncomingPacket(unsigned char *buffer, unsigned short length){// todo skovat praci s bufferem do knihovny
 unsigned char src_mac[MAC_ADDRESS_SIZE];
 memcpy(src_mac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE);
 if(arp_packet_is_arp(buffer, ARP_OPCODE_REQUEST_V)){
  arp_send_reply(buffer, src_mac);
  return;
 }
 if(!ip_packet_is_ip(buffer)){
  return;
 }
 unsigned char src_ip[4];
 memcpy(src_ip, buffer + IP_SRC_IP_P, 4);
 if(icmp_send_reply(buffer, length, src_mac, src_ip)){// moznost vypnuti icmp
  return;
 }
 unsigned short src_port;
 if(udp_is_udp( buffer, 5000, &src_port )){// upravit na nactaeni dest portu a predi do callbacku
  // pridani callbacku na prichozi packaket
  return;
 }
 if(TcpIsTcp(buffer)){
  TcpHandleIncomingPacket(buffer, length, src_mac, src_ip);
 }
}

void NetInit() {
 enc28j60_init();
 TcpInit();
}
#endif
