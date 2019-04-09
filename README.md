# AVR Net enc28j60

Library written in C for AVR microcontroller. This code is based on other public repository witch i do not remember where it is.
If someone will recognize code with is not mine and will know repository i will glad add reference here.

## Changes what i made.

* Fixes:
    * icmp ping with different length works, before library not response with same icmp length as was request but with constant length

* Improvements:
    * tcp can handle multiple established connections
    * synchronous wait for packet if receive different packet than expected will send received packed in to main loop function for handle except drop
    * arp broadcast responses are cached

## Requires

* Hardware
    * Some AVR microcontroller of course
    * enc28j60 chip
* Software
    * [AVR library](https://www.nongnu.org/avr-libc/)

## Usage

### Examples

[Tcp synchronous not persistent client](example/tcp_synch_client/main.c)

### General usage

```c
// define SPI pins connected to enc28j60 SI, SO, SCK pins
// SPI pins can changes on different AVR processors look into chip datasheet where SPI pins are
// this example is for AVR atmega328p
#define ENC28J60_DDR DDRB
#define ENC28J60_PORT PORTB
#define ENC28J60_SI_PIN_DDR DDB3
#define ENC28J60_SI_PIN PORTB3
#define ENC28J60_SO_PIN_DDR DDB4
#define ENC28J60_SCK_PIN_DDR DDB5
#define ENC28J60_SCK_PIN PORTB5

// define rest digitals I/O pins connected to enc28j60 RESET, INT, CS pins
// this pins have to be in same DDR and PORT as SPI pins
#define ENC28J60_RESET_PIN_DDR DDB0
#define ENC28J60_RESET_PIN PORTB0
#define ENC28J60_INT_PIN_DDR DDB1
#define ENC28J60_INT_PIN PORTB1
#define ENC28J60_CS_PIN_DDR DDB2
#define ENC28J60_CS_PIN PORTB2

// define IP, MAC address for you AVR
#define NET_IP 192, 168, 0, 150
#define NET_MAC 0x15, 0x16, 0x45, 0x89, 0x69, 0x99
// define RAM buffer size for network packets
// this define maximum size of incoming packet,
// define maximum size of sended packet,
// and define tcp max segment size
// bigger value allow send an receive bigger packet (UDP datagram) and better performance for TCP
// lower value save RAM space, you can create bigger buffer than AVR RAM or to big than no space for application :-)
#define NET_BUFFER_SIZE 400
// define size of arp cache array
// arp requests for ip are cached in RAM
// defined zero size turn off cache and code for cache is not included in compilation
// bigger value allow save more ip in cache is good if your AVR communicate with more hosts
// lower value is good for more dynamic network in smaller cache will often rewrite unused ip in cache
#define NET_ARP_CACHE_SIZE 5
// define how many tcp connection can live in AVR (server/client connections)
// if number connections is equal as limit all new incoming TCP connections is dropped and new client connection fail
// bigger value allow more connected hosts at one time
// lower value is RAM free
#define TCP_MAX_CONNECTIONS 2

// you have to define your cpu frequency because of delay.h library
#define F_CPU 16000000UL

// include icmp.c if you want your AVR to response PING
#include "src/icmp.c"
// include udp.c if you want handle incoming UDP datagram or send them
#include "src/udp.c"
// include tcp.c if you want handle incoming TCP connection or connect somewhere
#include "src/tcp.c"
// include one of icmp.c or udp.c or tcp.c otherwise AVR will only response on ARP and it is little useless :-)
// you have to include network.c (main include for library)
#include "src/network.c"

int main(){
 // at start of main function call NetInit for init enc28j60 chip init memory for cache...
 NetInit();
 for(;;){
  // in application loop everytime call NetHandleNetwork for handle incoming packet from ethernet
  NetHandleNetwork();
 }
}
```

### Callback functions

If you include tcp.c, you have to define functions callback TcpOnNewConnection, TcpOnConnect, TcpOnIncomingData, TcpOnDisconnect

```c
// function called if new tcp client is connecting into AVR, it is like firewall,
// library listen on all TCP ports you can specify witch port is accepted or not
// you have to return:
// NET_HANDLE_RESULT_DROP drop new connection without any response for sender
// NET_HANDLE_RESULT_OK accept new connection and allow established connection
// NET_HANDLE_RESULT_REJECT not allow established connection and library send icmp unreachable packet
unsigned char TcpOnNewConnection(const unsigned char connectionId){
 if(TcpGetConnection(connectionId)->port != 80){
  return NET_HANDLE_RESULT_DROP;
 }
 return NET_HANDLE_RESULT_OK;
}

// function called after successful established any connection
// server connection and client connection created by TcpConnect function
// purpose this callback s init any data or memory for new connection before send data
void TcpOnConnect(const unsigned char connectionId){
 // if you listen on more than one ports you always check port for execute correct logic
 // also if you use client connection from AVR anywhere,
 // check here you client connection id for handle asynchronous communication
 if(TcpGetConnection(connectionId)->port == 80){
 }
}

// function called always if AVR receive any data from any connection,
// server receive data from client or client receive asynchronous data
// data catched by synchronous wait is not send in this function
void TcpOnIncomingData(const unsigned char connectionId, const unsigned char *data, unsigned short dataLength){
 // here always check port if you listen on multiple ports or use client connection with asynchronous responses
 // so you can deliver incoming data in to correct logic
 if(TcpGetConnection(connectionId)->port == 80){
 }
}

// function called after closed any connection passively from other side or actively by TcpDisconnect function
// it is necessary for correct clear application memory after any connection ends
// if you prepare any memory in TcpOnConnect callback clear it here
void TcpOnDisconnect(const unsigned char connectionId){
 // always check port or client connection id here,
 // if you for different connection type prepare different memory you have to know what you have to clear
 // use this callback even if you use synchronous wait for response,
 // other side can close connection asynchronously sooner than you expect
 if(TcpGetConnection(connectionId)->port == 80){
 }
}
```

If you include udp.c, you have to define function callback UdpOnIncomingDatagram

```c
// function called any time if any UPD datagram income into AVR except UDP datagram cathed by synchronous wait
// you have to return:
// NET_HANDLE_RESULT_DROP inform library that your application code nothing do with datagram, nothing happened after
// NET_HANDLE_RESULT_OK inform library that your application code do somethink with datagram, nothing happened after
// NET_HANDLE_RESULT_REJECT inform library that your application code nothing do with datagram
// after that library send unreachable icmp packet
unsigned char UdpOnIncomingDatagram(UdpDatagram datagram, const unsigned char *data, unsigned short dataLength){
 if(datagram->port == 5000){
  // do something
  return NET_HANDLE_RESULT_OK;
 }
 return NET_HANDLE_RESULT_DROP;
}
```

### Functions

Functions from tcp.c TcpGetConnection, TcpConnect, TcpSendData, TcpReceiveData, TcpDisconnect

```c
// function return connection structure pointer with information about TCP connection remote ip, mac, port...
// if connection not found by connectionId return 0
const TcpConnection *TcpGetConnection(const unsigned char connectionId);

// function connect as client into server with milliseconds timeout
// return: TCP_INVALID_CONNECTION_ID in any error or after timeout on success return established connectionId
unsigned char TcpConnect(const unsigned char ip[IP_V4_ADDRESS_SIZE], const unsigned short remotePort, const unsigned short timeout);

// function send data into any established connection with timeout for TCP ack packet
// return: 1 on success, 0 on any error
// you can send bigger data than NET_BUFFER_SIZE if you do this data is send in multiple packet
unsigned char TcpSendData(const unsigned char connectionId, const unsigned short timeout, const unsigned char *data, unsigned short dataLength);

// synchronous waiting for data with timeout, after receive data is accessible by double pointer **data variable
// this function is good if you send data as client and waiting for response from server,
// not use this function in TcpOnIncomingData on parameter connectionId another data will came in next callback call
// receive data by this function will never be send into TcpOnIncomingData callback
// return: 1 on success, 0 on any error
unsigned char TcpReceiveData(const unsigned char connectionId, const unsigned short timeout, unsigned char **data, unsigned short *dataLength);

// actively close any TCP connection, you can disconnect inactive clients as server,
// or client have to notify server that ending communication
// return: 1 on success, 0 on any error
unsigned char TcpDisconnect(const unsigned char connectionId, const unsigned short timeout);
```

Functions from udp.c UdpSendDataMac, UdpSendData, UdpSendDataTmpPort, UdpReceiveData

```c
// send data into any host, you need know mac address,
// this function is good in UdpOnIncomingDatagram if you sending response as UDP server
// return:
// on success port used as srcPort in sended UDP datagram (port parameter)
// on fail return 0
unsigned short UdpSendDataMac(const unsigned char *mac, const unsigned char *ip, const unsigned short remotePort, const unsigned short port, const unsigned char *data, const unsigned short dataLength);

// send data into any host, you not need know mac address, mac address is lookup by ARP
// return:
// on success port used as srcPort in sended UDP datagram (port parameter)
// on fail return 0
unsigned short UdpSendData(const unsigned char *ip, const unsigned short remotePort, const unsigned short port, const unsigned char *data, const unsigned short dataLength);

// send data into any host, mac address is lookup and you specify only remotePort
// port (srcPort in sended UDP datagram is incrementing UDP port)
// this function is good for sending UDP request as client with synchronous waiting for response,
// you do not need store port value for long time
// return:
// on success port used as srcPort in sended UDP datagram
// (incrementing port, every sended UDP datagram rotate new port,
// also new client Tcp connection will cause incrementing)
// on fail return 0
// if you use this function instantly use UdpReceiveData for catch response with returned port value,
// or you lose it forever
unsigned short UdpSendDataTmpPort(const unsigned char *ip, const unsigned short remotePort, const unsigned char *data, const unsigned short dataLength);

// synchronous wait for data from specify host with timeout
// use it if you pray for response from server or any other host
// catched data by this function is not send into UdpOnIncomingDatagram callback
unsigned char UdpReceiveData(const unsigned char *ip, const unsigned short remotePort, const unsigned port, unsigned short timeout, unsigned char **data, unsigned short *dataLength);
```
