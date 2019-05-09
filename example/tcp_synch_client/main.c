/*

 This example show simple Tcp synchronous not persistent client communication.

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

// function called if new tcp client is connecting into AVR, it is like firewall,
unsigned char TcpOnNewConnection(const unsigned char connectionId){
 // this example is only client communication, every incoming tcp connection is droped (all clients try connect into AVR will timeout)
 return NET_HANDLE_RESULT_DROP;
}

// function called after successful established any connection
void TcpOnConnect(const unsigned char connectionId){
 // we use only synchronous client communication in this example this function can be empty
 // successfull connect will by detected by TcpConnect function
}

// function called always if AVR receive any data from any connection
void TcpOnIncomingData(const unsigned char connectionId, const unsigned char *data, unsigned short dataLength){
 // we use only synchronous client communication in this example this function can be empty
 // every data in this example will be readed synchronously by TcpReceiveData function
}

// function called after closed any connection
void TcpOnDisconnect(const unsigned char connectionId){
 // we use only synchronous client communication in this example this function can be empty
 // this example close connection after every request like HTTP 1.0
}

void TcpSynchNotPersitentClientExample(){
 unsigned char serverIp[IP_V4_ADDRESS_SIZE] = {192, 168, 0, 30};
 // client connect to server host (here example 192.168.0.30) and port 5000 with 6 second timeout
 unsigned char clientConId = TcpConnect(serverIp, 5000, 6000);
 // check if connect is success (can fail on timeout or if ip address not fount on local network)
 if(clientConId == TCP_INVALID_CONNECTION_ID){
  puts("Tcp connect error");
  return;
 }
 // send request message: "test_message" to server with 5 second timeout on tcp ack
 if(!TcpSendData(clientConId, 5000, "test_message", strlen("test_message"))){
  // if sending request data fail we try disconnect tcp connection and log into stdout failed request
  TcpDisconnect(clientConId, 1000);
  puts("Send request error");
  return;
 }
 unsigned char *responseData;
 unsigned short responseDataLength, i;
 // start loop for receive response on application protocol layer (we need loop until response message ends defined by zero character ASCII 0)
 for(;;){
  // synchronous wait for data from tcp connection with timeout 60 seconds
  if(!TcpReceiveData(clientConId, 60000, &responseData, &responseDataLength)){
   // if server do not response data before 60 seconds or any error we disconnect established tcp connection and log response error
   TcpDisconnect(clientConId, 1000);
   puts("Receive response error");
   return;
  }
  // instantly after receive data process them, because char pointer received in function TcpReceiveData is from network buffer
  // and if you use another network communication data can be rewritten!
  for(i=0; i<responseDataLength; i++){
   // if in response data is zero character ASCII 0 it is end of response message and we end waiting loop
   if(responseData[i] == 0){
    goto endLoop;
   }
   // if is not end of response message we send message char into stdout
   // in real application layer save response into application buffer and after read complete response process it
   putchar(responseData[i]);
  }
 }
 endLoop:
 // after successfull receive response message we disconnect established connection because clientConId is local variable
 // and we loose it after function end
 TcpDisconnect(clientConId, 5000);
 putchar('\n');
}

int main(){
 // init network library
 NetInit();
 for(;;){
  // handle incoming packet in application
  NetHandleNetwork();
  // every 1 second is called example request, here is delay only for simple example in real application use timers!
  // main loop have to always run for fast handling network connections
  TcpSynchNotPersitentClientExample();
  _delay_ms(1000);
 }
}
