#ifndef HTTP
#define HTTP

#include <stdio.h>
#include <string.h>

#define TCP_ON_NEW_CONNETION_CALLBACK HttpTcpOnNewConnection
#define TCP_ON_CONNECT_CALLBACK HttpTcpOnConnect
#define TCP_ON_INCOMING_DATA_CALLBACK HttpTcpOnIncomingData
#define TCP_ON_DISCONNECT_CALLBACK HttpTcpOnDisconnect

#include "tcp.c"

#ifndef HTTP_TCP_INCLUDED
#define HTTP_TCP_INCLUDED 0
#endif

#define HTTP_MAX_METHOD_LENGTH 10
#define HTTP_MAX_VERSION_LENGTH 10

#define HTTP_REQUEST_STATE_NO_REQUEST 0
#define HTTP_REQUEST_STATE_START_REQUEST 1
#define HTTP_REQUEST_STATE_METHOD 2
#define HTTP_REQUEST_STATE_URL 3
#define HTTP_REQUEST_STATE_VERSION 4

#define HTTP_STATE_LINUX_END_HEADER 5
#define HTTP_STATE_MAC_END_HEADER 6
#define HTTP_STATE_WIN_END_HEADER 7
#define HTTP_STATE_WIN_END_HEADER2 8
#define HTTP_STATE_HEADER 9
#define HTTP_STATE_END_HEADER 10
#define HTTP_STATE_END_MESSAGE 11

#define HTTP_REQUEST_STATE_REQUEST_HANDLING 12

#define HTTP_400_POS 1
#define HTTP_431_POS 2
#define HTTP_414_POS 3
#define HTTP_413_POS 4
#define HTTP_411_POS 5
#define HTTP_204_POS 6

typedef struct{
 const TcpConnection *connection;
 unsigned short headersLenght;
 unsigned char method[HTTP_MAX_METHOD_LENGTH+1];
 unsigned char url[HTTP_MAX_URL_LENGTH];
 unsigned char urlLength;
 unsigned char version[HTTP_MAX_VERSION_LENGTH+1];
 unsigned char headers[HTTP_MAX_HEADER_ROWS_LENGTH+1];
 unsigned char data[HTTP_MAX_DATA_LENGTH];
 unsigned char dataLength;
} HttpRequest;

typedef struct{
 unsigned short code;
 const char *message;
} HttpStatus;

typedef struct{
 unsigned short length;
 unsigned char *value;
} HttpHeaderValue;

#if HTTP_TCP_INCLUDED == 1
unsigned char TcpOnNewConnection(const unsigned char connectionId);
void TcpOnConnect(const unsigned char connectionId);
void TcpOnIncomingData(const unsigned char connectionId, const unsigned char *data, unsigned short dataLength);
void TcpOnDisconnect(const unsigned char connectionId);
#endif
void HttpOnIncomingRequest(const HttpRequest *request);

const static HttpStatus statuses[] = {
 {200, "OK"},
 {400, "Bad Request"},
 {431, "Request Header Fields Too Large"},
 {414, "URI Too Long"},
 {413, "Payload Too Large"},
 {411, "Length Required"},
 {204, "No Content"}
};
static unsigned char incomingRequestState = HTTP_REQUEST_STATE_NO_REQUEST;
static HttpRequest incomingRequest;
static unsigned char incomingRequestConnectionId = TCP_INVALID_CONNECTION_ID;
static unsigned short incomingPosition;

//*****************************************************************************************
//
// Function : HttpSendResponseHeader
// Description : build http response header and send it into tcp connection
//
//*****************************************************************************************
static unsigned char HttpSendResponseHeader(const unsigned char connectionId, const HttpStatus *status, unsigned char *headers, unsigned short headersLength, unsigned short dataLength){
 if(incomingRequestState == HTTP_REQUEST_STATE_NO_REQUEST){
  return 0;
 }
 incomingRequestState = HTTP_REQUEST_STATE_START_REQUEST;
 if(!status->message){
  unsigned char i;
  for(i=0; i<sizeof(statuses); i++){
   if(statuses[i].code == status->code){
    status = statuses + i;
    break;
   }
  }
 }
 unsigned char staticHeaders[51];
 int printResult = snprintf(staticHeaders, 50, "%s %u %s", incomingRequest.version, status->code, status->message ?: "Shit Happens");
 if(printResult < 0 || printResult >= 50){
  return 0;
 }
 strcat(staticHeaders, "\n");
 if(!TcpSendData(connectionId, 60000, staticHeaders, strlen(staticHeaders))){
  return 0;
 }
 if(headersLength){
  if(!TcpSendData(connectionId, 60000, headers, headersLength)){
   return 0;
  }
 }
 snprintf(staticHeaders, 51, "Connection: close\nContent-Length: %u\n\n", dataLength);
 if(!TcpSendData(connectionId, 60000, staticHeaders, strlen(staticHeaders))){
  return 0;
 }
 if(dataLength == 0){
  TcpDisconnect(connectionId, 5000);
 }
 return 1;
}

//*****************************************************************************************
//
// Function : HttpHeadersPutChar
// Description : put chat into request structure why parsing headers string on fail send error response header
//
//*****************************************************************************************
static unsigned char HttpHeadersPutChar(unsigned char ch){
 if(ch == '\n' && incomingRequest.headersLenght == 0){
  return 1;
 }
 if(incomingRequest.headersLenght >= HTTP_MAX_HEADER_ROWS_LENGTH){
  HttpSendResponseHeader(incomingRequestConnectionId, statuses + HTTP_431_POS, 0, 0, 0);
  return 0;
 }
 incomingRequest.headers[incomingRequest.headersLenght] = ch;
 incomingRequest.headersLenght++;
 return 1;
}

//*****************************************************************************************
//
// Function : HttpParseHeader
// Description : function take char from incoming data and decide where put them in request structure on fail send error response header
//
//*****************************************************************************************
static unsigned char HttpParseHeader(unsigned char ch, unsigned char *headerState){
 if(*headerState == HTTP_REQUEST_STATE_NO_REQUEST){
  return 0;
 }
 if(*headerState == HTTP_REQUEST_STATE_START_REQUEST){
  *headerState = HTTP_REQUEST_STATE_METHOD;
  incomingPosition = 0;
 }
 // parse http method
 if(*headerState == HTTP_REQUEST_STATE_METHOD){
  if(ch == ' '){
   *headerState = HTTP_REQUEST_STATE_URL;
   incomingRequest.method[incomingPosition] = 0;
   incomingRequest.urlLength = 0;
   return 1;
  }
  if(ch < 'A' || ch > 'Z'){
   HttpStatus status = {400, "Method Contains Illegal Chars"};
   HttpSendResponseHeader(incomingRequestConnectionId, &status, 0, 0, 0);
   return 0;
  }
  if(incomingPosition >= HTTP_MAX_METHOD_LENGTH){
   HttpStatus status = {431, "Method Too Long"};
   HttpSendResponseHeader(incomingRequestConnectionId, &status, 0, 0, 0);
   return 0;
  }
  incomingRequest.method[incomingPosition] = ch;
  incomingPosition++;
  return 1;
 }
 // parse url
 if(*headerState == HTTP_REQUEST_STATE_URL){
  if(ch == ' '){
   *headerState = HTTP_REQUEST_STATE_VERSION;
   incomingPosition = 0;
   return 1;
  }
  if(incomingRequest.urlLength >= HTTP_MAX_URL_LENGTH){
   HttpSendResponseHeader(incomingRequestConnectionId, statuses + HTTP_414_POS, 0, 0, 0);
   return 0;
  }
  incomingRequest.url[incomingRequest.urlLength] = ch;
  incomingRequest.urlLength++;
  return 1;
 }
 // parse http version
 if(*headerState == HTTP_REQUEST_STATE_VERSION){
  if(ch == '\r' || ch == '\n'){
   incomingRequest.version[incomingPosition] = 0;
   incomingRequest.headersLenght = 0;
   if(ch == '\r'){
    *headerState = HTTP_STATE_MAC_END_HEADER;
    return 1;
   }
   if(ch == '\n'){
    *headerState = HTTP_STATE_LINUX_END_HEADER;
    return 1;
   }
  }
  if(incomingPosition >= HTTP_MAX_VERSION_LENGTH){
   HttpStatus status = {431, "Version Too Long"};
   HttpSendResponseHeader(incomingRequestConnectionId, &status, 0, 0, 0);
   return 0;
  }
  incomingRequest.version[incomingPosition] = ch;
  incomingPosition++;
  return 1;
 }
 // parse headers end
 if(*headerState == HTTP_STATE_MAC_END_HEADER){
  if(ch == '\n'){
   *headerState = HTTP_STATE_WIN_END_HEADER;
   return 1;
  }
  if(ch == '\r'){
   *headerState = HTTP_STATE_END_HEADER;
   incomingRequest.dataLength = 0;
   incomingRequest.headers[incomingRequest.headersLenght] = 0;
   return 1;
  }
  *headerState = HTTP_STATE_HEADER;
  if(!HttpHeadersPutChar(ch)){
   return 0;
  }
  return 1;
 }
 if(*headerState == HTTP_STATE_LINUX_END_HEADER || *headerState == HTTP_STATE_WIN_END_HEADER2){
  if(ch == '\n'){
   *headerState = HTTP_STATE_END_HEADER;
   incomingRequest.dataLength = 0;
   incomingRequest.headers[incomingRequest.headersLenght] = 0;
   return 1;
  }
  *headerState = HTTP_STATE_HEADER;
  if(!HttpHeadersPutChar(ch)){
   return 0;
  }
  return 1;
 }
 if(*headerState == HTTP_STATE_WIN_END_HEADER){
  if(ch == '\r'){
   *headerState = HTTP_STATE_WIN_END_HEADER2;
   return 1;
  }
  *headerState = HTTP_STATE_HEADER;
  if(!HttpHeadersPutChar(ch)){
   return 0;
  }
  return 1;
 }
 // parse header rows
 if(*headerState == HTTP_STATE_HEADER){
  if(ch == '\r'){
   *headerState = HTTP_STATE_MAC_END_HEADER;
   if(!HttpHeadersPutChar('\n')){
    return 0;
   }
   return 1;
  }
  if(ch == '\n'){
   *headerState = HTTP_STATE_LINUX_END_HEADER;
  }
  if(!HttpHeadersPutChar(ch)){
   return 0;
  }
  return 1;
 }
 HttpStatus status = {400, "Headers Syntax Error"};
 HttpSendResponseHeader(incomingRequestConnectionId, &status, 0, 0, 0);
 return 0;
}

//*****************************************************************************************
//
// Function : HttpParseHeaderValue
// Description : function parse header value from request
// return HttpHeaderValue structure with 0 pointer value property if header not found or header row have wrong syntax
//
//*****************************************************************************************
const HttpHeaderValue HttpParseHeaderValue(const HttpRequest *request, const unsigned char *header){
 HttpHeaderValue headerValue;
 headerValue.value = strstr(request->headers, header);
 if(!headerValue.value){
  headerValue.length = 0;
  return headerValue;
 }
 headerValue.value += strlen(header);
 headerValue.length = request->headersLenght - (headerValue.value - request->headers);
 if(!headerValue.length){
  headerValue.value = 0;
  return headerValue;
 }
 if(headerValue.value[0] != ':'){
  headerValue.value = 0;
  headerValue.length = 0;
  return headerValue;
 }
 if(headerValue.length >= 2 && headerValue.value[1] == ' '){
  headerValue.value += 2;
  headerValue.length -= 2;
 }else{
  headerValue.value++;
  headerValue.length--;
 }
 unsigned short i;
 for(i=0; i<headerValue.length; i++){
  if(headerValue.value[i] == '\n'){
   headerValue.length = i;
   break;
  }
 }
 return headerValue;
}

//*****************************************************************************************
//
// Function : HttpSendResponse
// Description : function send response for client, can be used only once per request handling
//
//*****************************************************************************************
unsigned char HttpSendResponse(const HttpStatus *status, unsigned char *headers, unsigned short headersLength, unsigned char *data, unsigned short dataLength){
 if(incomingRequestState != HTTP_REQUEST_STATE_REQUEST_HANDLING){
  return 0;
 }
 unsigned char connectionId = incomingRequestConnectionId;
 if(!HttpSendResponseHeader(connectionId, status, headers, headersLength, dataLength)){
  return 0;
 }
 if(dataLength == 0){
  return 1;
 }
 if(!TcpSendData(connectionId, 60000, data, dataLength)){
  return 0;
 }
 TcpDisconnect(connectionId, 5000);
 return 1;
}

//*****************************************************************************************
//
// Function : HttpTcpOnNewConnection
// Description : defined tcp callback for accept new incoming connection on http port
//
//*****************************************************************************************
unsigned char HttpTcpOnNewConnection(const unsigned char connectionId){
 const TcpConnection *connection = TcpGetConnection(connectionId);
 if(connection->port != HTTP_SERVER_PORT){
  #if HTTP_TCP_INCLUDED == 1
  return TcpOnNewConnection(connectionId);
  #else
  return NET_HANDLE_RESULT_DROP;
  #endif
 }
 if(incomingRequestState != HTTP_REQUEST_STATE_NO_REQUEST){
  return NET_HANDLE_RESULT_DROP;
 }
 incomingRequestConnectionId = connectionId;
 incomingRequestState = HTTP_REQUEST_STATE_START_REQUEST;
 incomingPosition = 0;
 incomingRequest.connection = connection;
 return NET_HANDLE_RESULT_OK;
}

//*****************************************************************************************
//
// Function : HttpTcpOnConnect
// Description : defined tcp callback for detect new connection not used in http
//
//*****************************************************************************************
void HttpTcpOnConnect(const unsigned char connectionId){
 #if HTTP_TCP_INCLUDED == 1
 if(TcpGetConnection(connectionId)->port == HTTP_SERVER_PORT){
  return;
 }
 TcpOnConnect(connectionId);
 #endif
}

//*****************************************************************************************
//
// Function : HttpTcpOnIncomingData
// Description : handle incoming http request by tcp callback
//
//*****************************************************************************************
void HttpTcpOnIncomingData(const unsigned char connectionId, const unsigned char *data, unsigned short dataLength){
 if(TcpGetConnection(connectionId)->port != HTTP_SERVER_PORT){
  #if HTTP_TCP_INCLUDED == 1
  TcpOnIncomingData(connectionId, data, dataLength);
  #endif
  return;
 }
 if(incomingRequestState == HTTP_REQUEST_STATE_NO_REQUEST || connectionId != incomingRequestConnectionId){
  TcpDisconnect(connectionId, 5000);
  return;
 }
 // parse http headers
 unsigned short dataPosition = 0;
 if(incomingRequestState < HTTP_STATE_END_HEADER){
  for(dataPosition=0; dataPosition<dataLength; dataPosition++){
   if(!HttpParseHeader(data[dataPosition], &incomingRequestState)){
    return;
   }
   if(incomingRequestState == HTTP_STATE_END_HEADER){
    dataPosition++;
    break;
   }
  }
  if(incomingRequestState != HTTP_STATE_END_HEADER){
   return;
  }
 }
 // read request body
 if(incomingRequestState < HTTP_STATE_END_MESSAGE){
  if(strcmp(incomingRequest.method, "HEAD") == 0 || strcmp(incomingRequest.method, "GET") == 0 || strcmp(incomingRequest.method, "OPTIONS") == 0){
   incomingRequestState = HTTP_STATE_END_MESSAGE;
  }else{
   const HttpHeaderValue contentLength = HttpParseHeaderValue(&incomingRequest, "Content-Length");
   if(!contentLength.value){
    HttpSendResponseHeader(connectionId, statuses + HTTP_411_POS, 0, 0, 0);
    return;
   }
   unsigned long httpDataLength;
   if(!ParseLong(&httpDataLength, contentLength.value, contentLength.length)){
    HttpStatus status = {400, "Header Content-Length Syntax Error"};
    HttpSendResponseHeader(connectionId, &status, 0, 0, 0);
    return;
   }
   if(httpDataLength > HTTP_MAX_DATA_LENGTH){
    HttpSendResponseHeader(connectionId, statuses + HTTP_413_POS, 0, 0, 0);
    return;
   }
   dataLength -= dataPosition;
   data += dataPosition;
   memcpy(incomingRequest.data + incomingRequest.dataLength, data, dataLength);
   incomingRequest.dataLength += dataLength;
   if(incomingRequest.dataLength == httpDataLength){
    incomingRequestState = HTTP_STATE_END_MESSAGE;
   }else{
    return;
   }
  }
 }
 // handle request and send response
 if(incomingRequestState == HTTP_STATE_END_MESSAGE){
  incomingRequestState = HTTP_REQUEST_STATE_REQUEST_HANDLING;
  HttpOnIncomingRequest(&incomingRequest);
  if(incomingRequestState == HTTP_REQUEST_STATE_REQUEST_HANDLING){
   HttpSendResponseHeader(connectionId, statuses + HTTP_204_POS, 0, 0, 0);
   return;
  }
  return;
 }
 HttpStatus status = {400, "Request is in invalid state"};
 HttpSendResponseHeader(connectionId, &status, 0, 0, 0);
 return;
}

//*****************************************************************************************
//
// Function : HttpTcpOnDisconnect
// Description : after handling request reset memory and be ready for another http request
//
//*****************************************************************************************
void HttpTcpOnDisconnect(const unsigned char connectionId){
 if(TcpGetConnection(connectionId)->port != HTTP_SERVER_PORT){
  #if HTTP_TCP_INCLUDED == 1
  TcpOnDisconnect(connectionId);
  #endif
  return;
 }
 if(connectionId != incomingRequestConnectionId || incomingRequestState == HTTP_REQUEST_STATE_NO_REQUEST){
  return;
 }
 incomingRequestConnectionId = TCP_INVALID_CONNECTION_ID;
 incomingRequestState = HTTP_REQUEST_STATE_NO_REQUEST;
 return;
}
#endif
