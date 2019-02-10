#include <string.h>
#include "util.c"
#include "enc28j60.c"
#include "ethernet.c"
#include "arp.c"
#include "ip.c"
#include "icmp.h"
#include "udp.h"
#include "tcp.c"
#include "network.h"
// todo predelat nacitani hlavicek a komentare
void NetInit() {
 enc28j60_init();
 TcpInit();
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
 unsigned char src_mac[MAC_ADDRESS_SIZE];
 memcpy(src_mac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE);
 if(arp_packet_is_arp(buffer, ARP_OPCODE_REQUEST_V)){
  arp_send_reply(buffer, src_mac);
  return;
 }
 if(!ip_packet_is_ip(buffer)){
  return;
 }
 unsigned char src_ip[IP_V4_ADDRESS_SIZE];
 memcpy(src_ip, buffer + IP_SRC_IP_P, IP_V4_ADDRESS_SIZE);
 if(icmp_send_reply(buffer, length, src_mac, src_ip)){//todo moznost vypnuti icmp
  return;
 }
 unsigned short src_port;
 if(udp_is_udp( buffer, 5000, &src_port )){//todo upravit na nactaeni dest portu a predi do callbacku
  // pridani callbacku na prichozi packaket
  return;
 }
 if(TcpIsTcp(buffer)){
  TcpHandleIncomingPacket(buffer, length, src_mac, src_ip);
 }
}
