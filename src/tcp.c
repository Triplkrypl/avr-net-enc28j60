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
//static DWORD_BYTES tcp_sequence_number;

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
// todo prepsat s vyzadanou dylkou
//********************************************************************************************
//
// Function : tcp_puts_data
// Description : puts data from RAM to tx buffer
//
//********************************************************************************************
WORD tcp_puts_data ( BYTE *rxtx_buffer, BYTE *data, WORD offset )
{
	while( *data )
	{
		rxtx_buffer[ TCP_DATA_P + offset ] = *data++;
		offset++;
	}

	return offset;
}

// todo prejmenovat a mozna static
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

//********************************************************************************************
//
// Function : TcpSendPacket
// Description : send tcp packet to network.
//
//********************************************************************************************
static void TcpSendPacket(unsigned char *rxtx_buffer, const TcpConnection connection, unsigned char flags, unsigned short dlength, unsigned char *dest_mac){
	BYTE i, tseq;
	WORD_BYTES ck;

	// generate ethernet header
	eth_generate_header ( rxtx_buffer, (WORD_BYTES){ETH_TYPE_IP_V}, dest_mac );

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
    TcpSetPort(rxtx_buffer, connection.dest_port, connection.src_port);

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
	ck.word = software_checksum( &rxtx_buffer[IP_SRC_IP_P], sizeof(TCP_HEADER)+dlength+8, IP_PROTO_TCP_V + sizeof(TCP_HEADER) + dlength );
	rxtx_buffer[ TCP_CHECKSUM_H_P ] = ck.byte.high;
	rxtx_buffer[ TCP_CHECKSUM_L_P ] = ck.byte.low;

	// send packet to ethernet media
	enc28j60_packet_send ( rxtx_buffer, sizeof(ETH_HEADER)+sizeof(IP_HEADER)+sizeof(TCP_HEADER)+dlength );
}
// toto uvidome jetsli jejeste potreba mozna jo (refactor)
unsigned char tcp_send_ack(unsigned char *buffer, unsigned long sequence, unsigned short dataLength, TcpConnection *conection, unsigned char *dest_mac){
 unsigned char expectedData = sequence == conection->expectedSequence;
 if(expectedData){
  conection->expectedSequence += dataLength;
 }
 TcpSendPacket(buffer, *conection, TCP_FLAG_ACK_V, 0, dest_mac);
 return expectedData;
}

inline unsigned char TcpIsConnection(const TcpConnection connection, const unsigned char ip[4], const unsigned short destPort, const unsigned short srcPort){
 return connection.src_port == srcPort && connection.dest_port == destPort && memcmp(connection.ip, ip, 4) == 0 && connection.state != TCP_STATE_NO_CONNECTION;
}

static unsigned char TcpGetConnectionId(unsigned char ip[4], unsigned short destPort, unsigned short srcPort, unsigned char firstPacket){
 unsigned char i, empty = TCP_INVALID_CONNECTION_ID;
 for(i=0;i<TCP_MAX_CONNECTIONS; i++){
  if(TcpIsConnection(tcp_connections[i], ip, destPort, srcPort)){
   return i;
  }
  if(tcp_connections[i].state == TCP_STATE_NO_CONNECTION){
   empty = i;
  }
 }
 if(empty == TCP_INVALID_CONNECTION_ID || ! firstPacket){
  return TCP_INVALID_CONNECTION_ID;
 }
 tcp_connections[empty].state = TCP_STATE_NEW;
 tcp_connections[empty].dest_port = destPort;
 tcp_connections[empty].src_port = srcPort;
 memcpy(tcp_connections[empty].ip, ip, 4);
 return empty;
}

// todo komentar na ack 0
static unsigned char tcp_wait_packet(unsigned char *buffer, TcpConnection *connection, unsigned short *waiting, unsigned short timeout, unsigned short sendedDataLength, unsigned char expectedFlag){
 unsigned short length;
 unsigned short src_port;
 unsigned long ack_packet_seq;
 unsigned long ack_packet_ack;

 length = ethWaitPacket(buffer, ETH_TYPE_IP_V, waiting, timeout);
 if(length != 0){
  if(ip_packet_is_ip(buffer) && tcp_is_tcp(buffer, connection->dest_port, &src_port)){
   unsigned char srcMac[6];
   unsigned short tcp_length = tcp_get_dlength(buffer);
   memcpy(srcMac, buffer + ETH_SRC_MAC_P, 6);
   tcp_get_sequence(buffer, &ack_packet_seq, &ack_packet_ack);
   if(
    TcpIsConnection(*connection, buffer + IP_SRC_IP_P, connection->dest_port, src_port) &&
    ((buffer[ TCP_FLAGS_P ] & expectedFlag) || expectedFlag == 0) &&
    (tcp_length != 0 || expectedFlag != 0) &&
    connection->expectedSequence == ack_packet_seq &&
    (connection->sendSequence + sendedDataLength) == ack_packet_ack
   ){
    connection->sendSequence = ack_packet_ack;
    if(tcp_length != 0 && expectedFlag != 0) {
     tcp_handle_incoming_packet(buffer, length, srcMac, connection->ip, src_port, connection->dest_port);
    }
    return 1;
   }else{
    tcp_handle_incoming_packet(buffer, length, srcMac, connection->ip, src_port, connection->dest_port);
   }
  }else{
   NetHandleIncomingPacket(buffer, length);
  }
 }
 return 0;
}





TcpConnection tcp_connect(unsigned char *buffer, unsigned char *desIp, unsigned short destPort, unsigned short timeout){
 TcpConnection connection;
 // todo dodelat
}

static unsigned char tcp_close(unsigned char *buffer, TcpConnection *connection, unsigned char *destMac, unsigned short timeout){
 if(connection->state != TCP_STATE_DYEING){
  return 0;
 }
 unsigned short waiting = 0;
 for(;;){
  TcpSendPacket(buffer, *connection, TCP_FLAG_FIN_V, 0, destMac);
  if(tcp_wait_packet(buffer, connection, &waiting, 300, 1, TCP_FLAG_ACK_V)){
   return 1;
  }
  waiting += 300;
  if(waiting > timeout){
   break;
  }
 }
 return 1;
}

unsigned char tcp_send_data(unsigned char *buffer, TcpConnection *connection, unsigned char *destMac, unsigned short timeout, unsigned char *data, unsigned short dataLength){
 if(connection->state != TCP_STATE_ESTABLISHED){
  return 0;
 }
 unsigned short waiting = 0;
 for(;;){
  tcp_puts_data(buffer, data, 0);
  TcpSendPacket(buffer, *connection, TCP_FLAG_ACK_V, dataLength, destMac);
  if(tcp_wait_packet(buffer, connection, &waiting, 300, dataLength, TCP_FLAG_ACK_V)){
   return 1;
  }
  waiting += 300;
  if(waiting > timeout){
   break;
  }
 }
 return 0;
}
// todo comment dark magic
void tcp_handle_incoming_packet(unsigned char buffer[], unsigned short length, unsigned char srcMac[], unsigned char srcIp[], unsigned short srcPort, unsigned short destPort){
 unsigned char conID = TcpGetConnectionId(srcIp, destPort, srcPort, buffer[TCP_FLAGS_P] & TCP_FLAG_SYN_V);
 if(conID == TCP_INVALID_CONNECTION_ID){
  return;
 }
 unsigned long seq;
 unsigned long ack;
 tcp_get_sequence(buffer, &seq, &ack);
 unsigned short data_length = tcp_get_dlength(buffer);
 char out[100];
 sprintf(out, "Id conID %u state %u\n", conID, tcp_connections[conID].state);
 UARTWriteChars(out);
 sprintf(out, "Sec %u\n", seq);
 UARTWriteChars(out);
 sprintf(out, "Ack %u\n", ack);
 UARTWriteChars(out);
 sprintf(out, "Sec conection %u\n", tcp_connections[conID].sendSequence);
 UARTWriteChars(out);
 sprintf(out, "Expected Sec conection %u\n", tcp_connections[conID].expectedSequence);
 UARTWriteChars(out);
 sprintf(out, "Delka dat %u\n", data_length);
 UARTWriteChars(out);
 if((buffer[ TCP_FLAGS_P ] & TCP_FLAG_SYN_V) && (tcp_connections[conID].state == TCP_STATE_NEW || tcp_connections[conID].state == TCP_STATE_SYN_RECEIVED)){
  tcp_connections[conID].state = TCP_STATE_SYN_RECEIVED;
  tcp_connections[conID].expectedSequence = seq + 1;
  tcp_connections[conID].sendSequence = 2;
  sprintf(out, "SYN packet\n");
  UARTWriteChars(out);
  TcpSendPacket(buffer, tcp_connections[conID], TCP_FLAG_SYN_V|TCP_FLAG_ACK_V, 0, srcMac);
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
  tcp_send_ack(buffer, seq, 1, tcp_connections + conID, srcMac);
  sprintf(out, "Zadost o konec \n");
  UARTWriteChars(out);
  if(tcp_connections[conID].state == TCP_STATE_ESTABLISHED){
   tcp_connections[conID].state = TCP_STATE_DYEING;
   // pridat callback na ukoncene spojeni
   tcp_close(buffer, tcp_connections + conID, srcMac, 900);
   tcp_connections[conID].state = TCP_STATE_NO_CONNECTION;
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
 if(!tcp_send_ack(buffer, seq, data_length, tcp_connections + conID, srcMac)){
  return;
 }
 // pridani callbacku na prichozi data
 UARTWriteChars("Prichozi data '");
 UARTWriteCharsLength(buffer + TCP_SRC_PORT_H_P + tcp_get_hlength(buffer), data_length);
 UARTWriteChars("'\n");

 if(tcp_send_data(buffer, tcp_connections + conID, srcMac, 5000, buffer + TCP_SRC_PORT_H_P + tcp_get_hlength(buffer), data_length)){
  UARTWriteChars("Odchozi data OK\n");
 }
 return;
}

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
