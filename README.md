# AVR Net enc28j60

Library written in C for AVR microcontroller. This code is based on other public repository which i do not remember where it is.
If someone will recognize code which is not mine and will know repository i will glad add reference here.

## Versions

Current version **0.2.1**

```console
git clone -b 0.2.1 git@github.com:Triplkrypl/avr-net-enc28j60.git
```

Supported version **0.2**

[Changes](CHANGES.md)

## Requires

* Hardware
    * Some AVR microcontroller of course
    * enc28j60 chip
* Software
    * [AVR library](https://www.nongnu.org/avr-libc/)

## Usage

### Examples

[Tcp synchronous not persistent client](example/tcp_synch_client/main.c)

[Tcp asynchronous not persistent client](example/tcp_asynch_client/main.c)

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
#define NET_MAC 0x15, 0x8, 0x45, 0x89, 0x69, 0x99
// define RAM buffer size for network packets
// this define maximum size of incoming packet,
// define maximum size of sended packet,
// and define tcp max segment size
// bigger value allow send and receive bigger packet (UDP datagram) and better performance for TCP
// lower value save RAM, you can create bigger buffer than AVR RAM or to big than no space for application :-)
// minimum recommended value is 600 bytes, in IPV4 standard minimum IP datagram size is 576 bytes
// you can use smaller buffer, if you sure than you do not receive bigger packets
#define NET_BUFFER_SIZE 600
// define size of arp cache array, default value 5
// arp requests for ip are cached in RAM
// defined zero size turn off cache and code for cache is not included in compilation
// bigger value allow save more ip in cache is good if your AVR communicate with more hosts
// lower value is good for more dynamic network in smaller cache will often rewrite unused ip in cache
#define NET_ARP_CACHE_SIZE 5
// define for tcp.c include
// define how many tcp connection can live in AVR (server/client connections)
// if number connections is equal as limit all new incoming TCP connections are dropped,
// and TcpConnect function call will return TCP_INVALID_CONNECTION_ID
// bigger value allow more connected hosts at one time
// lower value is RAM free
#define TCP_MAX_CONNECTIONS 2
// define for http.c include
// define which tcp port is used for http listener
#define HTTP_SERVER_PORT 80
// definition of maximum request parts length, whole received request can be bigger than
// NET_BUFFER_SIZE because request can be received in multiple tcp packet but be ware out of memory
// define maximum request url length in bytes without host, example "/some/url?somequery"
#define HTTP_MAX_URL_LENGTH 50
// define maximum request headers length without first request header line where is method...
#define HTTP_MAX_HEADER_ROWS_LENGTH 50
// define maximum request body length
#define HTTP_MAX_DATA_LENGTH 50
// turn on/off callback from tcp.c, if http.c is included
// http.c define all callbacks from tcp.c so can not be used without define HTTP_TCP_INCLUDED as 1
// if tcp callbacks is turn on have to be defined in application as if tcp.c is included
// if you want use only http protocol you do not need this define and use default value 0
// if you want implement other protocol which used tcp and use http to, turn tcp callback on
// all tcp callback is not call on packet related to http protocol, this packets are handled inside http.c
#define HTTP_TCP_INCLUDED 0
// you can define end of all outgoing http header rows by default is used windows row ending,
// change only if you know than application on other side accept other row ending
//#define HTTP_HEADER_ROW_BREAK "\r\n"

// you have to define your cpu frequency because of delay.h library
#define F_CPU 16000000UL

// include icmp.c if you want your AVR to response PING
#include "src/icmp.c"
// include udp.c if you want handle incoming UDP datagram or send them
//#include "src/udp.c"
// include tcp.c if you want handle incoming TCP connection or connect somewhere
//#include "src/tcp.c"
// include http.c if you want handle incoming HTTP requests or send them
//#include "src/http.c"
// include one of icmp.c or udp.c or tcp.c or http.c,
// otherwise AVR will only response on ARP and it is little useless :-)
// not include tcp.c and http.c both if you want use both correctly,
// include http.c and before define HTTP_TCP_INCLUDED as 1
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

If you include **tcp.c**, you have to **define** functions callback **TcpOnNewConnection**, **TcpOnConnect**, **TcpOnIncomingData**, **TcpOnDisconnect**
You also have to define this callbacks if you include http.c and define HTTP_TCP_INCLUDED as 1

```c
// function called if new tcp client is connecting into AVR, it is like firewall,
// library listen on all TCP ports you can specify witch port is accepted or not
// you have to return:
// NET_HANDLE_RESULT_DROP drop new connection without any response for sender
// NET_HANDLE_RESULT_OK accept new connection and allow established connection
// NET_HANDLE_RESULT_REJECT not allow established connection and library send icmp unreachable packet
// icmp packet is send only if icmp.c is included
// without icmp protocol NET_HANDLE_RESULT_REJECT behave as NET_HANDLE_RESULT_DROP
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

If you include **udp.c**, you have to **define** function callback **UdpOnIncomingDatagram**

```c
// function called any time if any UPD datagram income into AVR except UDP datagram cathed by synchronous wait
// you have to return:
// NET_HANDLE_RESULT_DROP inform library that your application code nothing do with datagram
// NET_HANDLE_RESULT_OK inform library that your application code do somethink with datagram
// NET_HANDLE_RESULT_REJECT inform library that your application code nothing do with datagram
// after that library send unreachable icmp packet, icmp packet is send only if icmp.c is included
// without icmp protocol NET_HANDLE_RESULT_REJECT behave as NET_HANDLE_RESULT_DROP
unsigned char UdpOnIncomingDatagram(const UdpDatagram datagram, const unsigned char *data, unsigned short dataLength){
 if(datagram.port == 5000){
  // do something
  return NET_HANDLE_RESULT_OK;
 }
 return NET_HANDLE_RESULT_DROP;
}
```

If you include **http.c**, you have to **define** function callback **HttpOnIncomingRequest**
```c
// function called any time if http request if fully parsed and ready to be processed
// only request incoming with destination HTTP_SERVER_PORT is allow for parsing and passed in this callback
// response must be send by function HttpSendResponse called inside this callback
// response is not returned as return value because otherwise is dynamic allocation needed
// if HttpSendResponse is not called, library close tcp connection without send any response after callback ends
// any 404 and 500 or other applications errors have to be handled in application
// request analysis in application must be made before any other http communication,
// global memory allocated for request is reused for save memory on stack and data can be modified
void HttpOnIncomingRequest(const HttpRequest* request){
 if(strcmp(request->method, "GET") == 0 && CharsCmp(request->url, request->urlLength, "/some-url", strlen("/some-url"))){
  // do some logic
  HttpStatus status = {200, "OK"};
  HttpSendResponse(&status, "SomeHeader: :-)" HTTP_HEADER_ROW_BREAK, strlen("SomeHeader: :-)" HTTP_HEADER_ROW_BREAK), "SomeData", strlen("SomeData"));
  return;
 }
 HttpStatus status = {404, "Not Found"};
 HttpSendResponse(&status, 0, 0, 0, 0);
}
```

### Functions

Functions from **tcp.c TcpGetConnection**, **TcpConnect**, **TcpSendData**, **TcpReceiveData**, **TcpDisconnect**

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

// synchronous waiting for data with milliseconds timeout, zero timeout value means infinite waiting,
// in infinite timeout, function ends only if receive data or connection is closed
// after receive data are accessible by double pointer **data parameter
// this function is good if you send data as client and waiting for response from server,
// not use this function in TcpOnIncomingData on parameter connectionId another data will came in next callback call
// receive data by this function will never be send into TcpOnIncomingData callback
// return: 1 on success, 0 on any error
unsigned char TcpReceiveData(const unsigned char connectionId, const unsigned short timeout, unsigned char **data, unsigned short *dataLength);

// actively close any TCP connection, you can disconnect inactive clients as server,
// or client have to notify server that ending communication
// return: 1 on success, 0 on any error
unsigned char TcpDisconnect(const unsigned char connectionId, unsigned short timeout);
```

Functions from **udp.c UdpSendDataMac**, **UdpSendData**, **UdpSendDataTmpPort**, **UdpReceiveData**

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

Functions from **http.c HttpParseHeaderValue**, **HttpSendResponse**, **HttpSendRequest**

```c
// function parse value from http header rows, first parameter is http message as source of headers
// second parameter is header row key name for look up
// if header key name are more than once in message first one value is returned
// if header key name is not found in message, structure with zero length and zero pointer value is returned
// example: HttpParseHeaderValue(message, "Host"); with http headers:
// Host: somewhere.org
// SomeHeader: aaaaaaa
//
// will return structure with 13 length and "somewhere.org" value pointing into message structure without ending zero char
const HttpHeaderValue HttpParseHeaderValue(const HttpMessage *message, const unsigned char *header);

// function send http response into network while is some request processed in callback HttpOnIncomingRequest
// function return 1 on success 0 on any error
// can be called only once per callback call and only if request is processed, in other http listener internal state return 0 as error
// if HttpStatus.message is 0 pointer default status message is filed by library
// headers parameter is all additional response header rows example:
// "ContentType: json" HTTP_HEADER_ROW_BREAK
// "Language: en" HTTP_HEADER_ROW_BREAK
// all header rows have to end with HTTP_HEADER_ROW_BREAK string value
// usage of defined constant HTTP_HEADER_ROW_BREAK is important due to consistent rows end sent by library and rows end in application
// headersLength parameter is length of headers parameter in bytes, if zero length is set no additional headers are send and headers parameter is ignored
// data parameter is data send in response body if dataLength is zero no body is send and data parameter is ignored
unsigned char HttpSendResponse(const HttpStatus *status, unsigned char *headers, unsigned short headersLength, unsigned char *data, unsigned short dataLength);

// function send http request and synchronously wait for response
// return HttpResponse pointer on any error HttpResponse.status.code is 0 and HttpResponse.status.message contains simple error description
// connectionTimeout parameter define how long in milliseconds function wait for successful connect before send any request data
// requestTimeout parameter define how long function wait for response data, zero means infinite timeout
// method chars array end with zero character
// headers parameter contains all application request header rows example:
// "ContentType: json" HTTP_HEADER_ROW_BREAK
// all header rows must end with HTTP_HEADER_ROW_BREAK
// headersLength parameter length of application header rows in bytes
// data parameter is for request body content
// dataLength parameter length of body content in bytes
// HttpResponse analyze immediately before any other http communication,
// structure share memory with HttpRequest and structure data can be rewritten
const HttpResponse* HttpSendRequest(const unsigned char *ip, const unsigned short port, const unsigned short connectionTimeout, unsigned short requestTimeout, const unsigned char *method, const unsigned char *url, const unsigned char urlLength, const unsigned char *headers, const unsigned short headersLength, const unsigned char *data, unsigned short dataLength);
```

