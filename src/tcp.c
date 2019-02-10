//********************************************************************************************
//
// File : tcp.c implement for Transmission Control Protocol
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
#include <stdio.h>
#include "arp.h"
#include "tcp.h"
#include "ip.h"
#include "ethernet.h"
#include "network.h"
//********************************************************************************************
//
// +------------+-----------+------------+----------+
// + MAC header + IP header + TCP header + Data ::: +
// +------------+-----------+------------+----------+
//
// TCP Header
//
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +00+01+02+03+04+05+06+07+08+09+10+11+12+13+14+15+16+17+18+19+20+21+22+23+24+25+26+27+28+29+30+31+
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +               Source Port                     +                Destination Port               +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                        Sequence Number                                        +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                     Acknowledgment Number                                     +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +Data Offset+reserved+   ECN  +  Control Bits   +                  Window size                  +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                 Checksum                      +                Urgent Pointer                 +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                   Options and padding :::                                     +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// +                                             Data :::                                          +
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
//********************************************************************************************

static unsigned long connectPortRotaiting = TCP_MIN_DINAMIC_PORT;
static TcpConnection connections[TCP_MAX_CONNECTIONS];

//*****************************************************************************************
//
// Function : TcpInit
// Description : prepare necessary parts of memory for first run
//
//*****************************************************************************************
void TcpInit(){
 unsigned char i;
 for(i=0; i<TCP_MAX_CONNECTIONS; i++){
  connections[i].state = TCP_STATE_NO_CONNECTION;
 }
}

//*****************************************************************************************
//
// Function : TcpGetHeaderLength
// Description : claculate tcp received header length
//
//*****************************************************************************************
static inline unsigned char TcpGetHeaderLength(unsigned char *rxtx_buffer){
 return ((rxtx_buffer[ TCP_HEADER_LEN_P ]>>4) * 4); // generate len in bytes;
}

//*****************************************************************************************
//
// Function : TcpGetDataPosition
// Description : claculate tcp received data position in buffer
//
//*****************************************************************************************
static inline unsigned short TcpGetDataPosition(unsigned char *buffer){
 return TCP_SRC_PORT_H_P + TcpGetHeaderLength(buffer);
}

//*****************************************************************************************
//
// Function : TcpGetDataLength
// Description : claculate tcp received data length
//
//*****************************************************************************************
unsigned short TcpGetDataLength(unsigned char *rxtx_buffer){
	int dlength, hlength;

	dlength = ( rxtx_buffer[ IP_TOTLEN_H_P ] <<8 ) | ( rxtx_buffer[ IP_TOTLEN_L_P ] );
	dlength -= sizeof(IP_HEADER);
	hlength = TcpGetHeaderLength(rxtx_buffer);
	dlength -= hlength;
	if ( dlength <= 0 )
		dlength=0;

	return dlength;
}

//*****************************************************************************************
//
// Function : TcpGetSequence
// Description : get sequence and acknowledgment number from packet string
//
//*****************************************************************************************
static inline void TcpGetSequence(const unsigned char *buffer, unsigned long *seq, unsigned long *ack){
 ((unsigned char*)seq)[0] = buffer[TCP_SEQ_P + 3];
 ((unsigned char*)seq)[1] = buffer[TCP_SEQ_P + 2];
 ((unsigned char*)seq)[2] = buffer[TCP_SEQ_P + 1];
 ((unsigned char*)seq)[3] = buffer[TCP_SEQ_P];

 ((unsigned char*)ack)[0] = buffer[TCP_SEQACK_P + 3];
 ((unsigned char*)ack)[1] = buffer[TCP_SEQACK_P + 2];
 ((unsigned char*)ack)[2] = buffer[TCP_SEQACK_P + 1];
 ((unsigned char*)ack)[3] = buffer[TCP_SEQACK_P];
}

//*****************************************************************************************
//
// Function : TcpSetSequence
// Description : write sequence and acknowledgment number into packet string
//
//*****************************************************************************************
static inline void TcpSetSequence(unsigned char *buffer, const unsigned long seq, const unsigned long ack){
 buffer[TCP_SEQ_P + 3] = ((unsigned char*)&seq)[0];
 buffer[TCP_SEQ_P + 2] = ((unsigned char*)&seq)[1];
 buffer[TCP_SEQ_P + 1] = ((unsigned char*)&seq)[2];
 buffer[TCP_SEQ_P] = ((unsigned char*)&seq)[3];

 buffer[TCP_SEQACK_P + 3] = ((unsigned char*)&ack)[0];
 buffer[TCP_SEQACK_P + 2] = ((unsigned char*)&ack)[1];
 buffer[TCP_SEQACK_P + 1] = ((unsigned char*)&ack)[2];
 buffer[TCP_SEQACK_P] = ((unsigned char*)&ack)[3];
}

//*****************************************************************************************
//
// Function : TcpGetPort
// Description : get destination and source port from packet string
//
//*****************************************************************************************
static inline void TcpGetPort(const unsigned char *buffer, unsigned short *destination, unsigned short *source){
 ((unsigned char*)destination)[1] = buffer[TCP_DST_PORT_H_P];
 *((unsigned char*)destination) = buffer[TCP_DST_PORT_L_P];

 ((unsigned char*)source)[1] = buffer[TCP_SRC_PORT_H_P];
 *((unsigned char*)source) = buffer[TCP_SRC_PORT_L_P];
}

//*****************************************************************************************
//
// Function : TcpSetPort
// Description : write destination and source port into packet string
//
//*****************************************************************************************
static inline void TcpSetPort(unsigned char *buffer, const unsigned short destination, const unsigned short source){
 buffer[TCP_DST_PORT_H_P] = ((unsigned char*)&destination)[1];
 buffer[TCP_DST_PORT_L_P] = *((unsigned char*)&destination);

 buffer[TCP_SRC_PORT_H_P] = ((unsigned char*)&source)[1];
 buffer[TCP_SRC_PORT_L_P] = *((unsigned char*)&source);
}

//*****************************************************************************************
//
// Function : TcpSetChecksum
// Description : write checksum into packet string
//
//*****************************************************************************************
static inline void TcpSetChecksum(unsigned char *buffer, const unsigned short checksum){
 buffer[TCP_CHECKSUM_H_P] = ((unsigned char*)&checksum)[1];
 buffer[TCP_CHECKSUM_L_P] = *((unsigned char*)&checksum);
}

//********************************************************************************************
//
// Function : TcpSendPacket
// Description : send tcp packet to network.
//
//********************************************************************************************
static void TcpSendPacket(unsigned char *rxtx_buffer, const TcpConnection connection, const unsigned char flags, unsigned short dlength){
 eth_generate_header(rxtx_buffer, (WORD_BYTES){ETH_TYPE_IP_V}, connection.mac);
 if(flags & TCP_FLAG_SYN_V){
  // setup maximum segment size
  rxtx_buffer[ TCP_OPTIONS_P + 0 ] = 2;
  rxtx_buffer[ TCP_OPTIONS_P + 1 ] = 4;
  // size of receive buffer - tcp header, ip header and eth header size
  rxtx_buffer[ TCP_OPTIONS_P + 2 ] = high(MAX_RX_BUFFER - ETH_HEADER_LEN - IP_HEADER_LEN - TCP_HEADER_LEN);
  rxtx_buffer[ TCP_OPTIONS_P + 3 ] = low(MAX_RX_BUFFER - ETH_HEADER_LEN - IP_HEADER_LEN - TCP_HEADER_LEN);
  // setup tcp header length 24 bytes: 6*32/8 = 24
  rxtx_buffer[ TCP_HEADER_LEN_P ] = 0x60;
  dlength += 4;
 }else{
  // no options: 20 bytes: 5*32/8 = 20
  rxtx_buffer[TCP_HEADER_LEN_P] = 0x50;
 }
 ip_generate_header(rxtx_buffer, (WORD_BYTES){IP_HEADER_LEN + TCP_HEADER_LEN + dlength}, IP_PROTO_TCP_V, connection.ip);
 TcpSetSequence(rxtx_buffer, connection.sendSequence, connection.expectedSequence);
 TcpSetPort(rxtx_buffer, connection.remotePort, connection.port);
 // setup tcp flags
 rxtx_buffer[TCP_FLAGS_P] = flags;
 // setup maximum windows size
 rxtx_buffer[TCP_WINDOWSIZE_H_P] = high(MAX_RX_BUFFER - ETH_HEADER_LEN - IP_HEADER_LEN);
 rxtx_buffer[TCP_WINDOWSIZE_L_P] = low(MAX_RX_BUFFER - ETH_HEADER_LEN - IP_HEADER_LEN);
 // setup urgend pointer (not used -> 0)
 rxtx_buffer[TCP_URGENT_PTR_H_P] = 0;
 rxtx_buffer[TCP_URGENT_PTR_L_P] = 0;
 // clear old checksum and calculate new checksum
 rxtx_buffer[TCP_CHECKSUM_H_P] = 0;
 rxtx_buffer[TCP_CHECKSUM_L_P] = 0;
 // This is computed as the 16-bit one's complement of the one's complement
 // sum of a pseudo header of information from the
 // IP header, the TCP header, and the data, padded
 // as needed with zero bytes at the end to make a multiple of two bytes.
 // The pseudo header contains the following fields:
 //
 // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 // +00+01+02+03+04+05+06+07+08+09+10+11+12+13+14+15+16+17+18+19+20+21+22+23+24+25+26+27+28+29+30+31+
 // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 // +                                       Source IP address                                       +
 // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 // +                                     Destination IP address                                    +
 // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 // +           0           +      IP Protocol      +                    Total length               +
 // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 TcpSetChecksum(rxtx_buffer, software_checksum(rxtx_buffer + IP_SRC_IP_P, sizeof(TCP_HEADER)+dlength+8, IP_PROTO_TCP_V + sizeof(TCP_HEADER) + dlength));
 // send packet to ethernet media
 enc28j60_packet_send(rxtx_buffer, sizeof(ETH_HEADER)+sizeof(IP_HEADER)+sizeof(TCP_HEADER)+dlength);
}

//********************************************************************************************
//
// Function : TcpSendAck
// Description : send ack packet and detect if is first sending of ack
//
//********************************************************************************************
static unsigned char TcpSendAck(unsigned char *buffer, TcpConnection *connection, const unsigned long sequence, const unsigned short dataLength){
 unsigned char expectedData = sequence == connection->expectedSequence;
 if(expectedData){
  connection->expectedSequence += dataLength;
 }
 TcpSendPacket(buffer, *connection, TCP_FLAG_ACK_V, 0);
 return expectedData;
}

//********************************************************************************************
//
// Function : TcpIsTcp
// Description : check if packet in buffer is tcp packet
//
//********************************************************************************************
unsigned char TcpIsTcp(const unsigned char *rxtx_buffer){
 if(rxtx_buffer[IP_PROTO_P] == IP_PROTO_TCP_V){
  return 1;
 }
 return 0;
}

//********************************************************************************************
//
// Function : TcpGetConnectionInfo
// Description :
//
//********************************************************************************************
const TcpConnection *TcpGetConnection(const unsigned char connectionId){
 if(connectionId >= TCP_MAX_CONNECTIONS){
  return 0;
 }
 const TcpConnection *connection = connections + connectionId;
 if(connection->state == TCP_STATE_NO_CONNECTION){
  return 0;
 }
 return connection;
}

//********************************************************************************************
//
// Function : TcpIsConnection
// Description : compare if tcp connection and tcp connection parts is same
//
//********************************************************************************************
static inline unsigned char TcpIsConnection(const TcpConnection connection, const unsigned char mac[MAC_ADDRESS_SIZE], const const unsigned char ip[IP_V4_ADDRESS_SIZE], const unsigned short port, const unsigned short remotePort){
 return
  connection.state != TCP_STATE_NO_CONNECTION &&
  connection.port == port &&
  connection.remotePort == remotePort &&
  memcmp(connection.ip, ip, IP_V4_ADDRESS_SIZE) == 0 &&
  memcmp(connection.mac, mac, MAC_ADDRESS_SIZE) == 0
 ;
}

//********************************************************************************************
//
// Function : TcpGetEmptyConenctionId
// Description : get empty id for new connection
//
//********************************************************************************************
static unsigned char TcpGetEmptyConenctionId(){
 unsigned char i;
 for(i=0; i<TCP_MAX_CONNECTIONS; i++){
  if(connections[i].state == TCP_STATE_NO_CONNECTION){
   return i;
  }
 }
 return TCP_INVALID_CONNECTION_ID;
}

static unsigned char TcpGetConnectionId(const unsigned char mac[MAC_ADDRESS_SIZE], const unsigned char ip[IP_V4_ADDRESS_SIZE], const unsigned short port, const unsigned short remotePort, const unsigned char firstPacket){
 unsigned char i;
 for(i=0;i<TCP_MAX_CONNECTIONS; i++){
  if(TcpIsConnection(connections[i], mac, ip, port, remotePort)){
   return i;
  }
 }
 if(!firstPacket){
  return TCP_INVALID_CONNECTION_ID;
 }
 i = TcpGetEmptyConenctionId();
 if(i == TCP_INVALID_CONNECTION_ID){
  return TCP_INVALID_CONNECTION_ID;
 }
 connections[i].state = TCP_STATE_NEW;
 connections[i].port = port;
 connections[i].remotePort = remotePort;
 memcpy(connections[i].mac, mac, MAC_ADDRESS_SIZE);
 memcpy(connections[i].ip, ip, IP_V4_ADDRESS_SIZE);
 return i;
}

//********************************************************************************************
//
// Function : TcpWaitPacket
// Description : synchronous waiting for any tcp packet if sendedDataLength is zero function expect than waiting is for data packet
//
//********************************************************************************************
static unsigned char TcpWaitPacket(unsigned char *buffer, TcpConnection *connection, unsigned short sendedDataLength, unsigned char expectedFlag){
 unsigned short length = EthWaitPacket(buffer, ETH_TYPE_IP_V, 0);
 if(length != 0){
  if(ip_packet_is_ip(buffer) && TcpIsTcp(buffer)){
   unsigned short port, remotePort;
   unsigned long seq, ack;
   TcpGetPort(buffer, &port, &remotePort);
   TcpGetSequence(buffer, &seq, &ack);
   if(
    TcpIsConnection(*connection, buffer + ETH_SRC_MAC_P, buffer + IP_SRC_IP_P, port, remotePort) &&
    (buffer[ TCP_FLAGS_P ] & expectedFlag) &&
    ((connection->expectedSequence == seq) || (expectedFlag & TCP_FLAG_SYN_V)) &&
    (connection->sendSequence + sendedDataLength) == ack
   ){
    if(expectedFlag & TCP_FLAG_SYN_V){
     connection->expectedSequence = seq;
    }
    connection->sendSequence = ack;
    if(TcpGetDataLength(buffer) != 0 && sendedDataLength != 0) {
     TcpHandleIncomingPacket(buffer, length, connection->mac, connection->ip);
    }
    return 1;
   }else{
    unsigned char srcMac[MAC_ADDRESS_SIZE], srcIp[IP_V4_ADDRESS_SIZE];
    memcpy(srcMac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE);
    memcpy(srcIp, buffer + IP_SRC_IP_P, IP_V4_ADDRESS_SIZE);
    TcpHandleIncomingPacket(buffer, length, srcMac, srcIp);
   }
  }else{
   NetHandleIncomingPacket(buffer, length);
  }
 }
 return 0;
}

//********************************************************************************************
//
// Function : TcpConnect
// Description : active creation connection to server
//
//********************************************************************************************
unsigned char TcpConnect(unsigned char *buffer, const unsigned char ip[IP_V4_ADDRESS_SIZE], const unsigned short port, const unsigned short timeout){
 unsigned char connectionId = TcpGetEmptyConenctionId();
 if(connectionId == TCP_INVALID_CONNECTION_ID){
  return TCP_INVALID_CONNECTION_ID;
 }
 TcpConnection *connection = connections + connectionId;
 memcpy(connection->ip, ip, IP_V4_ADDRESS_SIZE);
 connection->port = connectPortRotaiting;
 connection->remotePort = port;
 connection->sendSequence = 2;
 connection->expectedSequence = 0;
 connection->state = TCP_STATE_NEW;
 connectPortRotaiting = (connectPortRotaiting == TCP_MAX_PORT) ? TCP_MIN_DINAMIC_PORT : connectPortRotaiting + 1;
 if(!ArpWhoIs(buffer, ip, connection->mac)){
  connection->state = TCP_STATE_NO_CONNECTION;
  return TCP_INVALID_CONNECTION_ID;
 }
 unsigned short waiting = 0;
 for(;;){
  if(waiting % 300 == 0){
   TcpSendPacket(buffer, *connection, TCP_FLAG_SYN_V, 0);
  }
  if(TcpWaitPacket(buffer, connection, 1, TCP_FLAG_SYN_V | TCP_FLAG_ACK_V)){
   TcpSendAck(buffer, connection, connection->expectedSequence, 1);// todo callback na nove spojeni
   connection->state = TCP_STATE_ESTABLISHED;
   return connectionId;
  }
  waiting++;
  if(waiting > timeout){
   break;
  }
 }
 connection->state = TCP_STATE_NO_CONNECTION;
 return TCP_INVALID_CONNECTION_ID;
}

//********************************************************************************************
//
// Function : TcpSendData
// Description : send data into tcp connection and synchronous wait for ACK with timeout
//
//********************************************************************************************
unsigned char TcpSendData(unsigned char *buffer, const unsigned char connectionId, const unsigned short timeout, const unsigned char *data, unsigned short dataLength){
 if(connectionId >= TCP_MAX_CONNECTIONS){
  return 0;
 }
 TcpConnection *connection = connections + connectionId;
 if(connection->state != TCP_STATE_ESTABLISHED){
  return 0;
 }
 // todo pocitat s max segment size
 unsigned short waiting = 0;
 for(;;){
  if(waiting % 300 == 0){
   memcpy(buffer + TCP_DATA_P, data, dataLength);
   TcpSendPacket(buffer, *connection, TCP_FLAG_ACK_V, dataLength);
  }
  if(TcpWaitPacket(buffer, connection, dataLength, TCP_FLAG_ACK_V)){
   return 1;
  }
  waiting++;
  if(waiting > timeout){
   break;
  }
 }
 return 0;
}

//********************************************************************************************
//
// Function : TcpReceiveData
// Description : synchronous wait for tcp data form connection with timeout
//
//********************************************************************************************
unsigned char TcpReceiveData(unsigned char *buffer, const unsigned char connectionId, const unsigned short timeout, unsigned char **data, unsigned short *dataLength){
 if(connectionId >= TCP_MAX_CONNECTIONS){
  return 0;
 }
 TcpConnection *connection = connections + connectionId;
 if(connection->state != TCP_STATE_ESTABLISHED){
  return 0;
 }
 unsigned short waiting = 0;
 for(;;){
  if(TcpWaitPacket(buffer, connection, 0, TCP_FLAG_ACK_V)){
   *data = buffer + TcpGetDataPosition(buffer);
   *dataLength = TcpGetDataLength(buffer);
   TcpSendAck(buffer, connection, connection->expectedSequence, *dataLength);
   return 1;
  }
  waiting++;
  if(waiting > timeout){
   break;
  }
 }
 return 0;
}

//********************************************************************************************
//
// Function : TcpClose
// Description : send fin packet and wait for him ack packet
//
//********************************************************************************************
static unsigned char TcpClose(unsigned char *buffer, TcpConnection *connection, const unsigned short timeout){
 if(connection->state != TCP_STATE_DYEING){
  return 0;
 }
 unsigned short waiting = 0;
 for(;;){
  if(waiting % 300 == 0){
   TcpSendPacket(buffer, *connection, TCP_FLAG_FIN_V | TCP_FLAG_ACK_V, 0);
  }
  if(TcpWaitPacket(buffer, connection, 1, TCP_FLAG_ACK_V)){
   return 1;
  }
  waiting++;
  if(waiting > timeout){
   break;
  }
 }
 return 1;
}
// todo vyresit zavolani callbacku pro konec
//********************************************************************************************
//
// Function : TcpDiconnect
// Description : close active connection client or server side (send fin packet and synchronous waiting for all packet with timeout)
//
//********************************************************************************************
unsigned char TcpDiconnect(unsigned char *buffer, const unsigned char connectionId, const unsigned short timeout){
 if(connectionId >= TCP_MAX_CONNECTIONS){
  return 0;
 }
 if(connections[connectionId].state != TCP_STATE_ESTABLISHED){
  return 0;
 }
 connections[connectionId].state = TCP_STATE_DYEING;
 TcpClose(buffer, connections + connectionId, 900);
 unsigned short waiting = 900;
 for(;;){
  if(TcpWaitPacket(buffer, connections + connectionId, 0, TCP_FLAG_FIN_V)){
   TcpSendAck(buffer, connections + connectionId, connections[connectionId].expectedSequence, 1);
   break;
  }
  waiting++;
  if(waiting > timeout){
   break;
  }
 }
 connections[connectionId].state = TCP_STATE_NO_CONNECTION;
 return 1;
}

// todo odebrat testovaci kod
//********************************************************************************************
//
// Function : TcpHandleIncomingPacket
// Description : function will passive processed any incoming tcp packet
//
//********************************************************************************************
void TcpHandleIncomingPacket(unsigned char *buffer, const unsigned short length, const unsigned char srcMac[MAC_ADDRESS_SIZE], const unsigned char srcIp[IP_V4_ADDRESS_SIZE]){
 unsigned char conID = TCP_INVALID_CONNECTION_ID;
 {
  unsigned short port, remotePort;
  TcpGetPort(buffer, &port, &remotePort);
  conID = TcpGetConnectionId(srcMac, srcIp, port, remotePort, buffer[TCP_FLAGS_P] == TCP_FLAG_SYN_V);
 }
 if(conID == TCP_INVALID_CONNECTION_ID){
  return;
 }
 unsigned long seq, ack;
 TcpGetSequence(buffer, &seq, &ack);
 unsigned short data_length = TcpGetDataLength(buffer);
 char out[100];
 sprintf(out, "Id conID %u state %u port %u remote port %u\n", conID, connections[conID].state, connections[conID].port, connections[conID].remotePort);
 UARTWriteChars(out);
 sprintf(out, "Sec %lu\n", seq);
 UARTWriteChars(out);
 sprintf(out, "Ack %lu\n", ack);
 UARTWriteChars(out);
 sprintf(out, "Sec conection %lu\n", connections[conID].sendSequence);
 UARTWriteChars(out);
 sprintf(out, "Expected Sec conection %lu\n", connections[conID].expectedSequence);
 UARTWriteChars(out);
 sprintf(out, "Delka dat %u\n", data_length);
 UARTWriteChars(out);
 if((buffer[ TCP_FLAGS_P ] == TCP_FLAG_SYN_V) && (connections[conID].state == TCP_STATE_NEW || connections[conID].state == TCP_STATE_SYN_RECEIVED)){
  if(connections[conID].state == TCP_STATE_NEW){
   if(connections[conID].port != 80){
    connections[conID].state = TCP_STATE_NO_CONNECTION;
    return;// todo neocekavany prichozi port zatim dropujem, vyresit icmp s odmitnutim
   }
   // todo pridani callbacku nove spojeni
  }
  connections[conID].state = TCP_STATE_SYN_RECEIVED;
  connections[conID].expectedSequence = seq + 1;
  connections[conID].sendSequence = 2;
  sprintf(out, "SYN packet\n");
  UARTWriteChars(out);
  TcpSendPacket(buffer, connections[conID], TCP_FLAG_SYN_V|TCP_FLAG_ACK_V, 0);
  return;
 }
 if((buffer[TCP_FLAGS_P] & TCP_FLAG_ACK_V) && connections[conID].state == TCP_STATE_SYN_RECEIVED){
  sprintf(out, "Established packet\n");
  UARTWriteChars(out);
  connections[conID].state = TCP_STATE_ESTABLISHED;
  connections[conID].sendSequence = ack;
 }
 if((buffer[TCP_FLAGS_P] & TCP_FLAG_FIN_V) && (connections[conID].state == TCP_STATE_ESTABLISHED || connections[conID].state == TCP_STATE_DYEING)){
  if(!TcpSendAck(buffer, connections + conID, seq, 1)){
   return;
  }
  sprintf(out, "Zadost o konec \n");
  UARTWriteChars(out);
  connections[conID].state = TCP_STATE_DYEING;
  // todo pridat callback na ukoncene spojeni
  TcpClose(buffer, connections + conID, 900);
  connections[conID].state = TCP_STATE_NO_CONNECTION;
  sprintf(out, "Konec\n");
  UARTWriteChars(out);
 }
 if(connections[conID].state != TCP_STATE_ESTABLISHED){
  return;
 }
 if(data_length == 0){
  return;
 }
 if(!TcpSendAck(buffer, connections + conID, seq, data_length)){
  return;
 }
 // todo pridani callbacku na prichozi data
 UARTWriteChars("Prichozi data '");
 UARTWriteCharsLength(buffer + TcpGetDataPosition(buffer), data_length);
 UARTWriteChars("'\n");

 unsigned char data[50];
 memcpy(data, buffer + TcpGetDataPosition(buffer), data_length);

 if(TcpSendData(buffer, conID, 5000, data, data_length)){
  UARTWriteChars("Odchozi data OK\n");
 }

 unsigned char testConnectIp[IP_V4_ADDRESS_SIZE] = {192, 168, 0, 30};
 unsigned char testConnectionId = TcpConnect(buffer, testConnectIp, 800, 2000);
 sprintf(out, "Nove spojeni %u\n", testConnectionId);
 UARTWriteChars(out);

 return;
}
