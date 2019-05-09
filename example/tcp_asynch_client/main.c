/*

 This example show simple Tcp asynchronous not persistent client communication.

 Application protocol is based on characters string messages end of message is defined by ASCII character 0 (null zero character).
 Client send one message as request after connect to server and server response with one message to client
 and after that client disconnect from server.

 Client logic is only send received message into stdout by stdio library.

 Definition of stdout is not in example, but can be find here:
 http://www.gnu.org/savannah-checkouts/non-gnu/avr-libc/user-manual/group__avr__stdio.html
 I recommend in example use uart serial line as stdout but can be anything (display what you use).

 Hardware configuration is here defined for AVR atmega328p with 16 MHz.

*/

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
#define NET_BUFFER_SIZE 400
// define maximum tcp connection can live in AVR (server/client connections)
#define TCP_MAX_CONNECTIONS 2

// define your cpu frequency because of delay.h library
#define F_CPU 16000000UL

// include tcp.c protocol functions
#include "../../src/tcp.c"
// include network.c (main include for library)
#include "../../src/network.c"
#include <stdio.h>

// declare global variable for store client connection so we can work with connection in all callbacks
// define it as TCP_INVALID_CONNECTION_ID and mark variable as nothing connection exist and we are not connected
// here we can declare more variables for detect multiple type of connection
unsigned char asynchClientConnectionId = TCP_INVALID_CONNECTION_ID;

// function called if new tcp client is connecting into AVR, it is like firewall,
unsigned char TcpOnNewConnection(const unsigned char connectionId){
 // this example is only client communication, every incoming tcp connection is droped (all clients try connect into AVR will timeout)
 return NET_HANDLE_RESULT_DROP;
}

// function called after successful established any connection
void TcpOnConnect(const unsigned char connectionId){
 // we use only asynchronous client communication in this example this function can be empty
 // successfull connect will by detected by TcpConnect function
}

// function called always if AVR receive any data from any connection
void TcpOnIncomingData(const unsigned char connectionId, const unsigned char *data, unsigned short dataLength){
 // if we waiting for response we have to handle it
 // handle data instantly before use any other communication data pointer is into ram network buffer
 if(connectionId == asynchClientConnectionId){
  // we must detect what response we handling, here i check it by connectionId
  // in this example we do not need any condition we are waiting only for one response at same time
  // but in real application we can wait for multiple responses and multiple types of response
  // so we must detect what real logic we want call for response
  // here in example only send response into stdout
  unsigned short i;
  for(i=0; i<dataLength; i++){
   // if in response data is zero character ASCII 0 it is end of response message and we close connection
   if(data[i] == 0){
    putchar('\n');
    TcpDisconnect(connectionId, 5000);
    // after disconnect we do not have to set global variable asynchClientConnectionId = TCP_INVALID_CONNECTION_ID
    // because we do it in TcpOnDisconnect witch is called if connection disconnecting
    break;
   }
   // if is not end of response message we send message char into stdout
   // in real application layer save response into application buffer and after read complete response process it
   putchar(data[i]);
  }
 }
}

// function called after closed any connection
void TcpOnDisconnect(const unsigned char connectionId){
 if(connectionId == asynchClientConnectionId){
  // we have to implement some logic in TcpOnDisconnect callback if we asynchronously waiting for response
  // response may not be delivered and we have to on disconnect clear memory
  // here in example only mark client connection as inactive and it is all, any tcp communication for disconnect is handled before or after callback
  asynchClientConnectionId = TCP_INVALID_CONNECTION_ID;
 }
}

void TcpAsynchNotPersitentClientExample(){
 unsigned char serverIp[IP_V4_ADDRESS_SIZE] = {192, 168, 0, 30};
 // client connect to server host (here example 192.168.0.30) and port 5000 with 6 second timeout
 asynchClientConnectionId = TcpConnect(serverIp, 5000, 6000);
 // check if connect is success (can fail on timeout or if ip address not fount on local network)
 if(asynchClientConnectionId == TCP_INVALID_CONNECTION_ID){
  puts("Tcp connect error");
  return;
 }
 // send request message: "test_message" to server with 5 second synchronous timeout on tcp ack
 if(!TcpSendData(asynchClientConnectionId, 5000, "test_message", strlen("test_message"))){
  // if sending request data fail we try disconnect tcp connection and log into stdout failed request
  TcpDisconnect(asynchClientConnectionId, 1000);
  // after we disconnect from server we save into global variable than connection is not active
  asynchClientConnectionId = TCP_INVALID_CONNECTION_ID;
  puts("Send request error");
  return;
 }
 // on success connection and deliver request we do nothing, all other logic (disconnect or receive response data) is handled in callbacks
}

int main(){
 // init network library
 NetInit();
 for(;;){
  // handle incoming packet in application
  NetHandleNetwork();
  // here is condition in mail loop for prevent duplicate waiting connection in same time for same request
  // after client connection is not exist we create instantly new with same request
  // request creation can be triggered by timer if we want call request periodically or any other event
  // if we waiting for response and we want create new same request we can disconnect waiting request
  // (maybe an timeout) and try request once more or we can have asynchClientConnectionId variable as array
  // and we can wait for multiple responses at one time but be ware of TCP_MAX_CONNECTIONS
  if(asynchClientConnectionId == TCP_INVALID_CONNECTION_ID){
   // every time in mail loop if asynchronous client connection is inactive or disconnected
   // we try connect and send request (connection and send request data is synchronous,
   // it can freeze mail loop for a while depend on ack packet latency)
   TcpAsynchNotPersitentClientExample();
  }
  // after successFull connection main loop run free and only receive response or disconnect will create new connection
  // here is infinite waiting for asynchronous response, we can start timer and if timer end
  // we can call TcpDisconnect and set asynchClientConnectionId = TCP_INVALID_CONNECTION_ID as inactive
 }
}
