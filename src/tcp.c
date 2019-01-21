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
#include "arp.h"
#include "tcp.h"
#include "ip.h"
#include "ethernet.h"
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

// global variables ***********************************************************************
TcpConnection tcp_connections[TCP_MAX_CONNECTIONS];

void TcpInit(){
 unsigned char i;
 for(i=0; i<TCP_MAX_CONNECTIONS; i++){
  tcp_connections[i].state = TCP_STATE_NO_CONNECTION;
 }
}
//*****************************************************************************************
//
// Function : tcp_get_dlength
// Description : claculate tcp received data length
//
//*****************************************************************************************
WORD tcp_get_dlength ( BYTE *rxtx_buffer )
{
	int dlength, hlength;

	dlength = ( rxtx_buffer[ IP_TOTLEN_H_P ] <<8 ) | ( rxtx_buffer[ IP_TOTLEN_L_P ] );
	dlength -= sizeof(IP_HEADER);
	hlength = (rxtx_buffer[ TCP_HEADER_LEN_P ]>>4) * 4; // generate len in bytes;
	dlength -= hlength;
	if ( dlength <= 0 )
		dlength=0;

	return ((WORD)dlength);
}
//*****************************************************************************************
//
// Function : tcp_get_hlength
// Description : claculate tcp received header length
//
//*****************************************************************************************
BYTE tcp_get_hlength ( BYTE *rxtx_buffer )
{
	return ((rxtx_buffer[ TCP_HEADER_LEN_P ]>>4) * 4); // generate len in bytes;
}

//********************************************************************************************
//
// Function : TcpPutsData
// Description : puts data from RAM to tx buffer
//
//********************************************************************************************
// todo mozna vyhodit
unsigned short TcpPutsData(unsigned char *rxtx_buffer, unsigned char *data, unsigned short dataLength, unsigned short offset){
 unsigned short i;
 for(i=0; i<dataLength; i++){
  rxtx_buffer[TCP_DATA_P + offset] = data[i];
  offset++;
 }
 return offset;
}

// todo prejmenovat a mozna static a mozna inline
void tcp_get_sequence(unsigned char *buffer, unsigned long *seq, unsigned long *ack){
 ((unsigned char*)seq)[0] = buffer[TCP_SEQ_P + 3];
 ((unsigned char*)seq)[1] = buffer[TCP_SEQ_P + 2];
 ((unsigned char*)seq)[2] = buffer[TCP_SEQ_P + 1];
 ((unsigned char*)seq)[3] = buffer[TCP_SEQ_P];

 ((unsigned char*)ack)[0] = buffer[TCP_SEQACK_P + 3];
 ((unsigned char*)ack)[1] = buffer[TCP_SEQACK_P + 2];
 ((unsigned char*)ack)[2] = buffer[TCP_SEQACK_P + 1];
 ((unsigned char*)ack)[3] = buffer[TCP_SEQACK_P];
}

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

static inline void TcpSetPort(unsigned char *buffer, const unsigned short destination, const unsigned short source){
 buffer[TCP_DST_PORT_H_P] = ((unsigned char*)&destination)[1];
 buffer[TCP_DST_PORT_L_P] = *((unsigned char*)&destination);

 buffer[TCP_SRC_PORT_H_P] = ((unsigned char*)&source)[1];
 buffer[TCP_SRC_PORT_L_P] = *((unsigned char*)&source);
}

// todo refactor drobnej
//********************************************************************************************
//
// Function : TcpSendPacket
// Description : send tcp packet to network.
//
//********************************************************************************************
static void TcpSendPacket(unsigned char *rxtx_buffer, const TcpConnection connection, unsigned char flags, unsigned short dlength){
	BYTE tseq;
	WORD_BYTES ck;

	eth_generate_header( rxtx_buffer, (WORD_BYTES){ETH_TYPE_IP_V}, connection.mac);

	// initial tcp sequence number
	// setup maximum segment size
	// require to setup first packet is receive or transmit only
	/*if ( max_segment_size )
	{
		// setup maximum segment size
		rxtx_buffer[ TCP_OPTIONS_P + 0 ] = 2;
		rxtx_buffer[ TCP_OPTIONS_P + 1 ] = 4;
		rxtx_buffer[ TCP_OPTIONS_P + 2 ] = HIGH(1408);
		rxtx_buffer[ TCP_OPTIONS_P + 3 ] = LOW(1408);
		// setup tcp header length 24 bytes: 6*32/8 = 24
		rxtx_buffer[ TCP_HEADER_LEN_P ] = 0x60;
		dlength += 4;
	}
	else
	{*/
		// no options: 20 bytes: 5*32/8 = 20
		rxtx_buffer[ TCP_HEADER_LEN_P ] = 0x50;
  //}

 ip_generate_header(rxtx_buffer, (WORD_BYTES){(sizeof(IP_HEADER) + sizeof(TCP_HEADER) + dlength)}, IP_PROTO_TCP_V, connection.ip);
 TcpSetSequence(rxtx_buffer, connection.sendSequence, connection.expectedSequence);
 TcpSetPort(rxtx_buffer, connection.remotePort, connection.port);

	// setup tcp flags
	rxtx_buffer [ TCP_FLAGS_P ] = flags;

	// setup maximum windows size
	rxtx_buffer [ TCP_WINDOWSIZE_H_P ] = HIGH((MAX_RX_BUFFER-sizeof(IP_HEADER)-sizeof(ETH_HEADER)));
	rxtx_buffer [ TCP_WINDOWSIZE_L_P ] = LOW((MAX_RX_BUFFER-sizeof(IP_HEADER)-sizeof(ETH_HEADER)));

	// setup urgend pointer (not used -> 0)
	rxtx_buffer[ TCP_URGENT_PTR_H_P ] = 0;
	rxtx_buffer[ TCP_URGENT_PTR_L_P ] = 0;

	// clear old checksum and calculate new checksum
	rxtx_buffer[ TCP_CHECKSUM_H_P ] = 0;
	rxtx_buffer[ TCP_CHECKSUM_L_P ] = 0;
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
	ck.word = software_checksum(rxtx_buffer + IP_SRC_IP_P, sizeof(TCP_HEADER)+dlength+8, IP_PROTO_TCP_V + sizeof(TCP_HEADER) + dlength );
	rxtx_buffer[ TCP_CHECKSUM_H_P ] = ck.byte.high;
	rxtx_buffer[ TCP_CHECKSUM_L_P ] = ck.byte.low;

 // send packet to ethernet media
 enc28j60_packet_send(rxtx_buffer, sizeof(ETH_HEADER)+sizeof(IP_HEADER)+sizeof(TCP_HEADER)+dlength);
}
// toto refactor
unsigned char TcpSendAck(unsigned char *buffer, unsigned long sequence, unsigned short dataLength, TcpConnection *conection){
 unsigned char expectedData = sequence == conection->expectedSequence;
 if(expectedData){
  conection->expectedSequence += dataLength;
 }
 TcpSendPacket(buffer, *conection, TCP_FLAG_ACK_V, 0);
 return expectedData;
}


// todo rozdelit zsikani portu a kontrolu tcp
unsigned char tcp_is_tcp(unsigned char *rxtx_buffer, unsigned short dest_port, unsigned short* src_port){
 if(rxtx_buffer[IP_PROTO_P] != IP_PROTO_TCP_V){
  return 0;
 }

 if(rxtx_buffer[TCP_DST_PORT_H_P] != ((unsigned char*)&dest_port)[1] || rxtx_buffer[TCP_DST_PORT_L_P] != *((unsigned char*)&dest_port)){
  return 0;
 }

 ((unsigned char*)src_port)[1] = rxtx_buffer[TCP_SRC_PORT_H_P];
 *((unsigned char*)src_port) = rxtx_buffer[TCP_SRC_PORT_L_P];

 return 1;
}

static inline unsigned char TcpIsConnection(const TcpConnection connection, unsigned char mac[MAC_ADDRESS_SIZE], const unsigned char ip[IP_V4_ADDRESS_SIZE], const unsigned short port, const unsigned short remotePort){
 return
  connection.state != TCP_STATE_NO_CONNECTION &&
  connection.port == port &&
  connection.remotePort == remotePort &&
  memcmp(connection.ip, ip, IP_V4_ADDRESS_SIZE) == 0 &&
  memcmp(connection.mac, mac, MAC_ADDRESS_SIZE) == 0
 ;
}

static unsigned char TcpGetConnectionId(unsigned char mac[6], unsigned char ip[4], unsigned short destPort, unsigned short srcPort, unsigned char firstPacket){
 unsigned char i;
 for(i=0;i<TCP_MAX_CONNECTIONS; i++){
  if(TcpIsConnection(tcp_connections[i], mac, ip, destPort, srcPort)){
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
 tcp_connections[i].state = TCP_STATE_NEW;
 tcp_connections[i].port = destPort;
 tcp_connections[i].remotePort = srcPort;
 memcpy(tcp_connections[i].mac, mac, MAC_ADDRESS_SIZE);
 memcpy(tcp_connections[i].ip, ip, IP_V4_ADDRESS_SIZE);
 return i;
}

// todo komentar na ack 0 refactor
static unsigned char TcpWaitPacket(unsigned char *buffer, TcpConnection *connection, unsigned short *waiting, unsigned short timeout, unsigned short sendedDataLength, unsigned char expectedFlag){
 unsigned short length;
 unsigned short srcPort;
 unsigned long ack_packet_seq;
 unsigned long ack_packet_ack;
 length = ethWaitPacket(buffer, ETH_TYPE_IP_V, waiting, timeout);
 if(length != 0){
  if(ip_packet_is_ip(buffer) && tcp_is_tcp(buffer, connection->port, &srcPort)){
   unsigned char srcMac[6];
   unsigned short tcp_length = tcp_get_dlength(buffer);
   memcpy(srcMac, buffer + ETH_SRC_MAC_P, 6);
   tcp_get_sequence(buffer, &ack_packet_seq, &ack_packet_ack);
   if(
    TcpIsConnection(*connection, buffer + ETH_SRC_MAC_P, buffer + IP_SRC_IP_P, connection->port, srcPort) &&
    ((buffer[ TCP_FLAGS_P ] & expectedFlag) || expectedFlag == 0) &&
    (tcp_length != 0 || expectedFlag != 0) &&// todo asi silenost
    ((connection->expectedSequence == ack_packet_seq) || (expectedFlag & TCP_FLAG_SYN_V)) &&
    (connection->sendSequence + sendedDataLength) == ack_packet_ack
   ){
    if(expectedFlag & TCP_FLAG_SYN_V){
     connection->expectedSequence = ack_packet_seq;
    }
    connection->sendSequence = ack_packet_ack;
    if(tcp_length != 0 && expectedFlag != 0) {
     tcp_handle_incoming_packet(buffer, length, srcMac, connection->ip, srcPort, connection->remotePort);
    }
    return 1;
   }else{
    tcp_handle_incoming_packet(buffer, length, srcMac, connection->ip, srcPort, connection->remotePort);
   }
  }else{
   NetHandleIncomingPacket(buffer, length);
  }
 }
 return 0;
}

//********************************************************************************************
//
// Function : TcpGetEmptyConenctionId
// Description : get empty id for new asynchronous connection
//
//********************************************************************************************
unsigned char TcpGetEmptyConenctionId(){
 unsigned char i;
 for(i=0; i<TCP_MAX_CONNECTIONS; i++){
  if(tcp_connections[i].state == TCP_STATE_NO_CONNECTION){
   return i;
  }
 }
 return TCP_INVALID_CONNECTION_ID;
}

// todo mozna jiny nazvi pormynch na vstupu
//********************************************************************************************
//
// Function : TcpConnect
// Description : active creation connection to server
//
//********************************************************************************************
void TcpConnect(unsigned char *buffer, TcpConnection *connection, const unsigned char *desIp, const unsigned short destPort, const unsigned short srcPort, const unsigned short timeout){
 memcpy(connection->ip, desIp, 4);
 connection->port = srcPort;
 connection->remotePort = destPort;
 connection->sendSequence = 2;
 connection->expectedSequence = 0;
 connection->state = TCP_STATE_NEW;
 if(!ArpWhoIs(buffer, desIp, connection->mac)){
  connection->state = TCP_STATE_NO_CONNECTION;
  return;
 }
 unsigned short waiting = 0;
 for(;;){
  TcpSendPacket(buffer, *connection, TCP_FLAG_SYN_V, 0);
  if(TcpWaitPacket(buffer, connection, &waiting, 300, 1, TCP_FLAG_SYN_V | TCP_FLAG_ACK_V)){
   TcpSendAck(buffer, connection->expectedSequence, 1, connection);
   connection->state = TCP_STATE_ESTABLISHED;
   return;
  }
  waiting += 300;
  if(waiting > timeout){
   break;
  }
 }
 connection->state = TCP_STATE_NO_CONNECTION;
}

unsigned char TcpSendData(unsigned char *buffer, TcpConnection *connection, unsigned short timeout, unsigned char *data, unsigned short dataLength){
 if(connection->state != TCP_STATE_ESTABLISHED){
  return 0;
 }
 unsigned short waiting = 0;
 for(;;){
  TcpPutsData(buffer, data, dataLength, 0);
  TcpSendPacket(buffer, *connection, TCP_FLAG_ACK_V, dataLength);
  if(TcpWaitPacket(buffer, connection, &waiting, 300, dataLength, TCP_FLAG_ACK_V)){
   return 1;
  }
  waiting += 300;
  if(waiting > timeout){
   break;
  }
 }
 return 0;
}

static unsigned char TcpClose(unsigned char *buffer, TcpConnection *connection, unsigned short timeout){
 if(connection->state != TCP_STATE_DYEING){
  return 0;
 }
 unsigned short waiting = 0;
 for(;;){
  TcpSendPacket(buffer, *connection, TCP_FLAG_FIN_V, 0);
  if(TcpWaitPacket(buffer, connection, &waiting, 300, 1, TCP_FLAG_ACK_V)){
   return 1;
  }
  waiting += 300;
  if(waiting > timeout){
   break;
  }
 }
 return 1;
}

// todo comment dark magic refactor
void tcp_handle_incoming_packet(unsigned char buffer[], unsigned short length, unsigned char srcMac[MAC_ADDRESS_SIZE], unsigned char srcIp[IP_V4_ADDRESS_SIZE], unsigned short srcPort, unsigned short destPort){
 unsigned char conID = TcpGetConnectionId(srcMac, srcIp, destPort, srcPort, buffer[TCP_FLAGS_P] == TCP_FLAG_SYN_V);
 if(conID == TCP_INVALID_CONNECTION_ID){
  return;
 }
 unsigned long seq;
 unsigned long ack;
 tcp_get_sequence(buffer, &seq, &ack);
 unsigned short data_length = tcp_get_dlength(buffer);
 char out[100];
 sprintf(out, "Id conID %u state %u port %u remote port %u\n", conID, tcp_connections[conID].state, tcp_connections[conID].port, tcp_connections[conID].remotePort);
 UARTWriteChars(out);
 sprintf(out, "Sec %lu\n", seq);
 UARTWriteChars(out);
 sprintf(out, "Ack %lu\n", ack);
 UARTWriteChars(out);
 sprintf(out, "Sec conection %lu\n", tcp_connections[conID].sendSequence);
 UARTWriteChars(out);
 sprintf(out, "Expected Sec conection %lu\n", tcp_connections[conID].expectedSequence);
 UARTWriteChars(out);
 sprintf(out, "Delka dat %u\n", data_length);
 UARTWriteChars(out);
 if((buffer[ TCP_FLAGS_P ] == TCP_FLAG_SYN_V) && (tcp_connections[conID].state == TCP_STATE_NEW || tcp_connections[conID].state == TCP_STATE_SYN_RECEIVED)){
  tcp_connections[conID].state = TCP_STATE_SYN_RECEIVED;
  tcp_connections[conID].expectedSequence = seq + 1;
  tcp_connections[conID].sendSequence = 2;
  sprintf(out, "SYN packet\n");
  UARTWriteChars(out);
  TcpSendPacket(buffer, tcp_connections[conID], TCP_FLAG_SYN_V|TCP_FLAG_ACK_V, 0);
  return;
 }
 if(tcp_connections[conID].expectedSequence != seq){
  return;
 }
 if((buffer[TCP_FLAGS_P] & TCP_FLAG_ACK_V) && tcp_connections[conID].state == TCP_STATE_SYN_RECEIVED){
  // pridani callbacku nove spojeni
  sprintf(out, "Established packet\n");
  UARTWriteChars(out);
  tcp_connections[conID].state = TCP_STATE_ESTABLISHED;
  tcp_connections[conID].sendSequence = ack;
 }
 if((buffer[TCP_FLAGS_P] & TCP_FLAG_FIN_V) && (tcp_connections[conID].state == TCP_STATE_ESTABLISHED || tcp_connections[conID].state == TCP_STATE_DYEING)){
  TcpSendAck(buffer, seq, 1, tcp_connections + conID);
  sprintf(out, "Zadost o konec \n");
  UARTWriteChars(out);
  if(tcp_connections[conID].state == TCP_STATE_ESTABLISHED){
   tcp_connections[conID].state = TCP_STATE_DYEING;
   // pridat callback na ukoncene spojeni
   TcpClose(buffer, tcp_connections + conID, 900);
   tcp_connections[conID].state = TCP_STATE_NO_CONNECTION;// mozna presunout do close
   sprintf(out, "Konec\n");
   UARTWriteChars(out);
  }
 }
 if(tcp_connections[conID].state != TCP_STATE_ESTABLISHED){
  return;
 }
 if(data_length == 0){
  return;
 }
 if(!TcpSendAck(buffer, seq, data_length, tcp_connections + conID)){
  return;
 }
 // pridani callbacku na prichozi data
 UARTWriteChars("Prichozi data '");
 UARTWriteCharsLength(buffer + TCP_SRC_PORT_H_P + tcp_get_hlength(buffer), data_length);
 UARTWriteChars("'\n");

 unsigned char data[50];
 memcpy(data, buffer + TCP_SRC_PORT_H_P + tcp_get_hlength(buffer), data_length);

 unsigned char testConnectionId = TcpGetEmptyConenctionId();
 if(testConnectionId == TCP_INVALID_CONNECTION_ID){
  return;
 }
 unsigned char testConnectIp[IP_V4_ADDRESS_SIZE] = {192, 168, 0, 30};
 TcpConnect(buffer, tcp_connections + testConnectionId, testConnectIp, 800, 7050, 2000);

 if(TcpSendData(buffer, tcp_connections + testConnectionId, 5000, data, data_length)){
  UARTWriteChars("Odchozi data OK\n");
 }
 return;
}
