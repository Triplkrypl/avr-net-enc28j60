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
#include "enc28j60.h"
#include "ip.h"
#include "arp.h"
#include "tcp.h"
#include "ethernet.h"
#include "network.h"
#include "util.c"
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

extern unsigned short connectPortRotaiting;
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
static inline unsigned char TcpGetHeaderLength(const unsigned char *buffer){
 return ((buffer[ TCP_HEADER_LEN_P ]>>4) * 4); // generate len in bytes;
}

//*****************************************************************************************
//
// Function : TcpGetDataPosition
// Description : claculate tcp received data position in buffer
//
//*****************************************************************************************
static inline unsigned short TcpGetDataPosition(const unsigned char *buffer){
 return TCP_SRC_PORT_H_P + TcpGetHeaderLength(buffer);
}

//*****************************************************************************************
//
// Function : TcpGetDataLength
// Description : claculate tcp received data length
//
//*****************************************************************************************
unsigned short TcpGetDataLength(const unsigned char *rxtx_buffer){
	int dlength, hlength;

	dlength = ( rxtx_buffer[ IP_TOTLEN_H_P ] <<8 ) | ( rxtx_buffer[ IP_TOTLEN_L_P ] );
	dlength -= IP_HEADER_LEN;
	hlength = TcpGetHeaderLength(rxtx_buffer);
	dlength -= hlength;
	if ( dlength <= 0 )
		dlength=0;

	return dlength;
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

//********************************************************************************************
//
// Function : TcpChecksum
// Description : calculate checksum for tcp datagram
//
//********************************************************************************************
static inline unsigned short TcpChecksum(const unsigned char *buffer){
 return software_checksum(buffer + IP_SRC_IP_P, TcpGetHeaderLength(buffer) + TcpGetDataLength(buffer) + 8, IP_PROTO_TCP_V + TcpGetHeaderLength(buffer) + TcpGetDataLength(buffer));
}

//********************************************************************************************
//
// Function : TcpSendPacket
// Description : send tcp packet to network.
//
//********************************************************************************************
static void TcpSendPacket(unsigned char *buffer, const TcpConnection *connection, unsigned long sendSequence, const unsigned char flags, unsigned short dlength){
 eth_generate_header(buffer, ETH_TYPE_IP_V, connection->mac);
 if(flags & TCP_FLAG_SYN_V){
  // setup maximum segment size
  buffer[ TCP_OPTIONS_P + 0 ] = 2;
  buffer[ TCP_OPTIONS_P + 1 ] = 4;
  // size of receive buffer - tcp header, ip header and eth header size
  buffer[ TCP_OPTIONS_P + 2 ] = High(TCP_MAX_SEGMENT_SIZE);
  buffer[ TCP_OPTIONS_P + 3 ] = Low(TCP_MAX_SEGMENT_SIZE);
  // setup tcp header length 24 bytes: 6*32/8 = 24
  buffer[ TCP_HEADER_LEN_P ] = 0x60;
  dlength += 4;
 }else{
  // no options: 20 bytes: 5*32/8 = 20
  buffer[TCP_HEADER_LEN_P] = 0x50;
 }
 ip_generate_header(buffer, IP_HEADER_LEN + TCP_HEADER_LEN + dlength, IP_PROTO_TCP_V, connection->ip);
 TcpSetSequence(buffer, sendSequence, connection->expectedSequence);
 TcpSetPort(buffer, connection->remotePort, connection->port);
 // setup tcp flags
 buffer[TCP_FLAGS_P] = flags;
 // setup maximum windows size
 buffer[TCP_WINDOWSIZE_H_P] = High(MAX_RX_BUFFER - ETH_HEADER_LEN - IP_HEADER_LEN);
 buffer[TCP_WINDOWSIZE_L_P] = Low(MAX_RX_BUFFER - ETH_HEADER_LEN - IP_HEADER_LEN);
 // setup urgend pointer (not used -> 0)
 buffer[TCP_URGENT_PTR_H_P] = 0;
 buffer[TCP_URGENT_PTR_L_P] = 0;
 // clear old checksum and calculate new checksum
 buffer[TCP_CHECKSUM_H_P] = 0;
 buffer[TCP_CHECKSUM_L_P] = 0;
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
 CharsPutShort(buffer + TCP_CHECKSUM_H_P, TcpChecksum(buffer));
 // send packet to ethernet media
 enc28j60_packet_send(buffer, ETH_HEADER_LEN + IP_HEADER_LEN + TCP_HEADER_LEN + dlength);
}

//********************************************************************************************
//
// Function : TcpVerifyChecksum
// Description : verify tcp datagram checksum of incoming data
//
//********************************************************************************************
static unsigned char TcpVerifyChecksum(unsigned char *buffer){
 unsigned short dataSum = CharsToShort(buffer + TCP_CHECKSUM_H_P);
 buffer[TCP_CHECKSUM_H_P] = 0;
 buffer[TCP_CHECKSUM_L_P] = 0;
 unsigned short sum = TcpChecksum(buffer);
 CharsPutShort(buffer + TCP_CHECKSUM_H_P, dataSum);
 return dataSum == sum;
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
 TcpSendPacket(buffer, connection, connection->sendSequence, TCP_FLAG_ACK_V, 0);
 return expectedData;
}

//********************************************************************************************
//
// Function : TcpGetOptionPosition
// Description : get tcp option position in options list return 0 if option is not set
//
//********************************************************************************************
static unsigned short TcpGetOptionPosition(const unsigned char *buffer, const unsigned char kind){
 unsigned short i, dataPos = TcpGetDataPosition(buffer);
 for(i=TCP_OPTIONS_P; i<dataPos;){
  if(buffer[i] == TCP_OPTION_END_LIST_KIND){
   break;
  }
  if(buffer[i] == kind){
   return i;
  }
  if(buffer[i] == TCP_OPTION_NO_OPERATION_KIND){
   i++;
   continue;
  }
  i = i + buffer[i+1];
 }
 return 0;
}

//********************************************************************************************
//
// Function : TcpGetConnection
// Description : return connection informations for tcp connection id return 0 pointer on not existing connection
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
static inline unsigned char TcpIsConnection(const unsigned char *buffer, const TcpConnection *connection, const unsigned short port, const unsigned short remotePort){
 return
  connection->state != TCP_STATE_NO_CONNECTION &&
  connection->port == port &&
  connection->remotePort == remotePort &&
  memcmp(connection->ip, buffer + IP_SRC_IP_P, IP_V4_ADDRESS_SIZE) == 0 &&
  memcmp(connection->mac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE) == 0
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

//********************************************************************************************
//
// Function : TcpGetConnectionId
// Description : get connection id for packet data
//
//********************************************************************************************
static unsigned char TcpGetConnectionId(const unsigned char *buffer){
 unsigned short port = CharsToShort(buffer + TCP_DST_PORT_P), remotePort = CharsToShort(buffer + TCP_SRC_PORT_P);
 unsigned char i;
 for(i=0;i<TCP_MAX_CONNECTIONS; i++){
  if(TcpIsConnection(buffer, connections + i, port, remotePort)){
   return i;
  }
 }
 if(buffer[TCP_FLAGS_P] != TCP_FLAG_SYN_V){
  return TCP_INVALID_CONNECTION_ID;
 }
 i = TcpGetEmptyConenctionId();
 if(i == TCP_INVALID_CONNECTION_ID){
  return TCP_INVALID_CONNECTION_ID;
 }
 connections[i].state = TCP_STATE_NEW;
 connections[i].port = port;
 connections[i].remotePort = remotePort;
 memcpy(connections[i].mac, buffer + ETH_SRC_MAC_P, MAC_ADDRESS_SIZE);
 memcpy(connections[i].ip, buffer + IP_SRC_IP_P, IP_V4_ADDRESS_SIZE);
 connections[i].expectedSequence = CharsToLong(buffer + TCP_SEQ_P) + 1;
 connections[i].sendSequence = 2;
 unsigned short optionPosition = TcpGetOptionPosition(buffer, TCP_OPTION_MAX_SEGMET_SIZE_KIND);
 connections[i].maxSegmetSize = optionPosition ? (CharsToShort(buffer + optionPosition + 2) ?: TCP_MAX_SEGMENT_SIZE) : TCP_MAX_SEGMENT_SIZE;
 if(connections[i].maxSegmetSize > TCP_MAX_SEGMENT_SIZE){ connections[i].maxSegmetSize = TCP_MAX_SEGMENT_SIZE; }
 return i;
}

//********************************************************************************************
//
// Function : TcpWaitPacket
// Description : synchronous waiting for any tcp packet if expectedAck is connection->sendSequence function expect than waiting is for data packet
//
//********************************************************************************************
static unsigned char TcpWaitPacket(unsigned char *buffer, TcpConnection *connection, unsigned long expectedAck, unsigned char expectedFlag){
 unsigned short length = EthWaitPacket(buffer, ETH_TYPE_IP_V, 0);
 if(length != 0){
  if(ip_packet_is_ip(buffer) && buffer[IP_PROTO_P] == IP_PROTO_TCP_V){
   if(!TcpVerifyChecksum(buffer)){
    return 0;
   }
   unsigned long seq = CharsToLong(buffer + TCP_SEQ_P);
   unsigned long ack = CharsToLong(buffer + TCP_SEQACK_P);
   if(
    TcpIsConnection(buffer, connection, CharsToShort(buffer + TCP_DST_PORT_P), CharsToShort(buffer + TCP_SRC_PORT_P)) &&
    (!((~buffer[ TCP_FLAGS_P ]) & expectedFlag)) &&
    ((connection->expectedSequence == seq) || (expectedFlag & TCP_FLAG_SYN_V)) &&
    expectedAck == ack
   ){
    if(expectedFlag & TCP_FLAG_SYN_V){
     connection->expectedSequence = seq;
    }
    if((TcpGetDataLength(buffer) != 0 || buffer[TCP_FLAGS_P] & TCP_FLAG_FIN_V) && connection->sendSequence == expectedAck) {
     TcpHandleIncomingPacket(buffer, length);
    }
    return 1;
   }
   TcpHandleIncomingPacket(buffer, length);
  }else{
   NetHandleIncomingPacket(length);
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
static void TcpClose(unsigned char *buffer, TcpConnection *connection, const unsigned short timeout){
 unsigned short waiting = 0;
 unsigned long sendSequence = connection->sendSequence;
 connection->sendSequence += 1;
 for(;;){
  if(waiting % 300 == 0){
   TcpSendPacket(buffer, connection, sendSequence, TCP_FLAG_FIN_V | TCP_FLAG_ACK_V, 0);
  }
  if(TcpWaitPacket(buffer, connection, sendSequence + 1, TCP_FLAG_ACK_V)){
   break;
  }
  waiting++;
  if(waiting > timeout){
   break;
  }
 }
 connection->state = TCP_STATE_NO_CONNECTION;
}

//********************************************************************************************
//
// Function : TcpConnect
// Description : active creation connection to server
//
//********************************************************************************************
unsigned char TcpConnect(const unsigned char ip[IP_V4_ADDRESS_SIZE], const unsigned short remotePort, const unsigned short timeout){
 unsigned char connectionId = TcpGetEmptyConenctionId();
 if(connectionId == TCP_INVALID_CONNECTION_ID){
  return TCP_INVALID_CONNECTION_ID;
 }
 TcpConnection *connection = connections + connectionId;
 memcpy(connection->ip, ip, IP_V4_ADDRESS_SIZE);
 connection->port = connectPortRotaiting;
 connection->remotePort = remotePort;
 connection->sendSequence = 2;
 connection->expectedSequence = 0;
 connection->state = TCP_STATE_NEW;
 connectPortRotaiting = (connectPortRotaiting == NET_MAX_PORT) ? NET_MIN_DINAMIC_PORT : connectPortRotaiting + 1;
 unsigned char *buffer = NetGetBuffer();
 if(!ArpWhoIs(buffer, ip, connection->mac)){
  connection->state = TCP_STATE_NO_CONNECTION;
  return TCP_INVALID_CONNECTION_ID;
 }
 unsigned short waiting = 0;
 unsigned long sendSequence = connection->sendSequence;
 connection->sendSequence += 1;
 for(;;){
  if(waiting % 300 == 0){
   TcpSendPacket(buffer, connection, sendSequence, TCP_FLAG_SYN_V, 0);
  }
  if(TcpWaitPacket(buffer, connection, sendSequence + 1, TCP_FLAG_SYN_V | TCP_FLAG_ACK_V)){
   unsigned short optionPosition = TcpGetOptionPosition(buffer, TCP_OPTION_MAX_SEGMET_SIZE_KIND);
   connection->maxSegmetSize = optionPosition ? (CharsToShort(buffer + optionPosition + 2) ?: TCP_MAX_SEGMENT_SIZE) : TCP_MAX_SEGMENT_SIZE;
   if(connection->maxSegmetSize > TCP_MAX_SEGMENT_SIZE){ connection->maxSegmetSize = TCP_MAX_SEGMENT_SIZE; }
   TcpSendAck(buffer, connection, connection->expectedSequence, 1);
   connection->state = TCP_STATE_ESTABLISHED;
   TCP_ON_CONNECT_CALLBACK(connectionId);
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
unsigned char TcpSendData(const unsigned char connectionId, const unsigned short timeout, const unsigned char *data, unsigned short dataLength){
 if(connectionId >= TCP_MAX_CONNECTIONS){
  return 0;
 }
 if(dataLength == 0){
  return 0;
 }
 TcpConnection *connection = connections + connectionId;
 unsigned short waiting = 0, offset = 0, partLength;
 unsigned char firstTry, *buffer = NetGetBuffer();
 do{
  firstTry = 1;
  partLength = connection->maxSegmetSize;
  if(dataLength - offset < partLength){
   partLength = dataLength - offset;
  }
  unsigned long sendSequence = connection->sendSequence;
  connection->sendSequence += partLength;
  for(;;){
   if(connection->state != TCP_STATE_ESTABLISHED){
    return 0;
   }
   if(waiting % 200 == 0 || firstTry){
    memcpy(buffer + TCP_DATA_P, data + offset, partLength);
    TcpSendPacket(buffer, connection, sendSequence, TCP_FLAG_ACK_V, partLength);
    firstTry = 0;
   }
   if(TcpWaitPacket(buffer, connection, sendSequence + partLength, TCP_FLAG_ACK_V)){
    break;
   }
   waiting++;
   if(waiting > timeout){
    return 0;
   }
  }
  offset = offset + partLength;
 }
 while(offset < dataLength);
 return 1;
}

//********************************************************************************************
//
// Function : TcpReceiveData
// Description : synchronous wait for tcp data from connection with timeout in milliseconds (zero timeout is infinite wait for any data)
//
//********************************************************************************************
unsigned char TcpReceiveData(const unsigned char connectionId, const unsigned short timeout, unsigned char **data, unsigned short *dataLength){
 if(connectionId >= TCP_MAX_CONNECTIONS){
  return 0;
 }
 TcpConnection *connection = connections + connectionId;
 unsigned short waiting = 0;
 unsigned char *buffer = NetGetBuffer();
 for(;;){
  if(connection->state != TCP_STATE_ESTABLISHED){
   return 0;
  }
  if(TcpWaitPacket(buffer, connection, connection->sendSequence, TCP_FLAG_ACK_V)){
   *data = buffer + TcpGetDataPosition(buffer);
   *dataLength = TcpGetDataLength(buffer);
   TcpSendAck(buffer, connection, connection->expectedSequence, *dataLength);
   return 1;
  }
  if(timeout > 0){
   waiting++;
   if(waiting > timeout){
    break;
   }
  }
 }
 return 0;
}

//********************************************************************************************
//
// Function : TcpDisconnect
// Description : close active connection client or server side (send fin packet and synchronous waiting for all packet with timeout)
//
//********************************************************************************************
unsigned char TcpDisconnect(const unsigned char connectionId, unsigned short timeout){
 if(connectionId >= TCP_MAX_CONNECTIONS){
  return 0;
 }
 if(connections[connectionId].state != TCP_STATE_ESTABLISHED){
  return 0;
 }
 connections[connectionId].state = TCP_STATE_DYEING;
 unsigned char *buffer = NetGetBuffer();
 TCP_ON_DISCONNECT_CALLBACK(connectionId);
 TcpClose(buffer, connections + connectionId, timeout);
 return 1;
}

//********************************************************************************************
//
// Function : TcpHandleIncomingPacket
// Description : function will passive processed any incoming tcp packet
//
//********************************************************************************************
void TcpHandleIncomingPacket(unsigned char *buffer, unsigned short length){
 if(!TcpVerifyChecksum(buffer)){
  return;
 }
 unsigned char connectionId = TcpGetConnectionId(buffer);
 if(connectionId == TCP_INVALID_CONNECTION_ID){
  return;
 }
 // handle new connection
 if((buffer[TCP_FLAGS_P] == TCP_FLAG_SYN_V) && (connections[connectionId].state == TCP_STATE_NEW || connections[connectionId].state == TCP_STATE_SYN_RECEIVED)){
  if(connections[connectionId].state == TCP_STATE_NEW){
   unsigned char result = TCP_ON_NEW_CONNETION_CALLBACK(connectionId);
   if(result == NET_HANDLE_RESULT_DROP){
    connections[connectionId].state = TCP_STATE_NO_CONNECTION;
    return;
   }
   if(result == NET_HANDLE_RESULT_REJECT){
    #ifdef ICMP
    IcmpSendUnreachable(buffer, connections[connectionId].mac, connections[connectionId].ip, CharsToShort(buffer + IP_TOTLEN_H_P));
    #endif
    connections[connectionId].state = TCP_STATE_NO_CONNECTION;
    return;
   }
  }
  connections[connectionId].state = TCP_STATE_SYN_RECEIVED;
  TcpSendPacket(buffer, connections + connectionId, connections[connectionId].sendSequence, TCP_FLAG_SYN_V|TCP_FLAG_ACK_V, 0);
  return;
 }
 if((buffer[TCP_FLAGS_P] & TCP_FLAG_ACK_V) && connections[connectionId].state == TCP_STATE_SYN_RECEIVED){
  connections[connectionId].state = TCP_STATE_ESTABLISHED;
  connections[connectionId].sendSequence = CharsToLong(buffer + TCP_SEQACK_P);
  TCP_ON_CONNECT_CALLBACK(connectionId);
 }
 // handle connection close
 if((buffer[TCP_FLAGS_P] & TCP_FLAG_FIN_V) && (connections[connectionId].state == TCP_STATE_ESTABLISHED || connections[connectionId].state == TCP_STATE_DYEING)){
  if(!TcpSendAck(buffer, connections + connectionId, CharsToLong(buffer + TCP_SEQ_P), 1)){
   return;
  }
  if(connections[connectionId].state == TCP_STATE_ESTABLISHED){
   connections[connectionId].state = TCP_STATE_DYEING;
   TCP_ON_DISCONNECT_CALLBACK(connectionId);
   TcpClose(buffer, connections + connectionId, 1000);
  }
 }
 if(connections[connectionId].state != TCP_STATE_ESTABLISHED){
  return;
 }
 length = TcpGetDataLength(buffer);
 // handle incoming data packet
 if(length == 0){
  return;
 }
 if(!TcpSendAck(buffer, connections + connectionId, CharsToLong(buffer + TCP_SEQ_P), length)){
  return;
 }
 TCP_ON_INCOMING_DATA_CALLBACK(connectionId, buffer + TcpGetDataPosition(buffer), length);
 return;
}
