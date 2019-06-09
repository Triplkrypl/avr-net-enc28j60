#include "tcp.c"
#include <stdio.h>
#include <string.h>

#define HTTP_MAX_METHOD_LENGTH 10
#define HTTP_MAX_VERSION_LENGTH 10

#define HTTP_REQUEST_STATE_NO_REQUEST 0
#define HTTP_REQUEST_STATE_START_REQUEST 1
#define HTTP_REQUEST_STATE_METHOD 2
#define HTTP_REQUEST_STATE_URL 3
#define HTTP_REQUEST_STATE_VERSION 4
#define HTTP_REQUEST_STATE_LINUX_END_HEADER 5
#define HTTP_REQUEST_STATE_MAC_END_HEADER 6
#define HTTP_REQUEST_STATE_WIN_END_HEADER 7
#define HTTP_REQUEST_STATE_WIN_END_HEADER2 8
#define HTTP_REQUEST_STATE_HEADER 9
#define HTTP_REQUEST_STATE_END_HEADER 10
#define HTTP_REQUEST_STATE_END_REQUEST 11
#define HTTP_REQUEST_STATE_RESPONSE_HEADER_SENT 12

typedef struct{
 unsigned char method[HTTP_MAX_METHOD_LENGTH+1];
 unsigned char url[HTTP_MAX_URL_LENGTH];
 unsigned char urlLength;
 unsigned char version[HTTP_MAX_VERSION_LENGTH+1];
 unsigned char headers[HTTP_MAX_HEADER_ROWS_LENGTH+1];
 unsigned char headersLenght;
 unsigned char data[HTTP_MAX_DATA_LENGTH];
 unsigned char dataLength;
} HttpRequest;

typedef struct{
 unsigned short code;
 unsigned char *message;
} HttpStatus;

typedef struct{
 unsigned short length;
 unsigned char *value;
} HttpHeaderValue;

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
static unsigned short incomingRequestPosition;

//*****************************************************************************************
//
// Function : HttpSendResponseHeader
// Description : build http response header and send it into tcp connection
//
//*****************************************************************************************
static unsigned char HttpSendResponseHeader(const unsigned char connectionId, unsigned short statusCode, unsigned char *statusMessage, unsigned char *headers, unsigned short headersLength, unsigned short dataLength){
 if(incomingRequestState == HTTP_REQUEST_STATE_NO_REQUEST){
  return 0;
 }
 if(!statusMessage){
  unsigned char i;
  for(i=0; i<sizeof(statuses); i++){
   if(statuses[i].code == statusCode){
    statusMessage = statuses[i].message;
    break;
   }
  }
  if(!statusMessage){
   statusMessage = "Shit Happens";
  }
 }
 unsigned char staticHeaders[52];
 int printResult = snprintf(staticHeaders, 50, "%s %u %s", incomingRequest.version, statusCode, statusMessage);
 if(printResult < 0 || printResult >= 50){
  return 0;
 }
 strcat(staticHeaders, "\r\n");
 if(!TcpSendData(connectionId, 60000, staticHeaders, strlen(staticHeaders))){
  return 0;
 }
 if(headersLength){
  if(!TcpSendData(connectionId, 60000, headers, headersLength)){
   return 0;
  }
 }
 snprintf(staticHeaders, 52, "Content-Length: %u\r\n\r\n", dataLength);
 if(!TcpSendData(connectionId, 60000, staticHeaders, strlen(staticHeaders))){
   return 0;
  }
 incomingRequestState = HTTP_REQUEST_STATE_RESPONSE_HEADER_SENT;
 return 1;
}

//*****************************************************************************************
//
// Function : HttpHeadersPutChar
// Description : put chat into request structure why parsing headers string
//
//*****************************************************************************************
static unsigned char HttpHeadersPutChar(unsigned char ch){
 if(ch == '\n' && incomingRequest.headersLenght == 0){
  return 1;
 }
 if(incomingRequest.headersLenght >= HTTP_MAX_HEADER_ROWS_LENGTH){
  HttpSendResponseHeader(incomingRequestConnectionId, 431, 0, "", 0, 0);
  return 0;
 }
 incomingRequest.headers[incomingRequest.headersLenght] = ch;
 incomingRequest.headersLenght++;
 return 1;
}

//*****************************************************************************************
//
// Function : HttpParseHeader
// Description : function take char from incoming data and decide where put them in request structure
//
//*****************************************************************************************
static unsigned char HttpParseHeader(unsigned char ch){
 if(incomingRequestState == HTTP_REQUEST_STATE_NO_REQUEST){
  return 0;
 }
 if(incomingRequestState == HTTP_REQUEST_STATE_START_REQUEST){
  incomingRequestState = HTTP_REQUEST_STATE_METHOD;
  incomingRequestPosition = 0;
 }
 // parse http method
 if(incomingRequestState == HTTP_REQUEST_STATE_METHOD){
  if(ch == ' '){
   incomingRequestState = HTTP_REQUEST_STATE_URL;
   incomingRequest.method[incomingRequestPosition] = 0;
   incomingRequest.urlLength = 0;
   return 1;
  }
  if(ch < 'A' || ch > 'Z'){
   HttpSendResponseHeader(incomingRequestConnectionId, 400, "Method Contains Illegal Chars", "", 0, 0);
   return 0;
  }
  if(incomingRequestPosition >= HTTP_MAX_METHOD_LENGTH){
   HttpSendResponseHeader(incomingRequestConnectionId, 431, "Method Too Long", "", 0, 0);
   return 0;
  }
  incomingRequest.method[incomingRequestPosition] = ch;
  incomingRequestPosition++;
  return 1;
 }
 // parse url
 if(incomingRequestState == HTTP_REQUEST_STATE_URL){
  if(ch == ' '){
   incomingRequestState = HTTP_REQUEST_STATE_VERSION;
   incomingRequestPosition = 0;
   return 1;
  }
  if(incomingRequest.urlLength >= HTTP_MAX_URL_LENGTH){
   HttpSendResponseHeader(incomingRequestConnectionId, 414, 0, "", 0, 0);
   return 0;
  }
  incomingRequest.url[incomingRequest.urlLength] = ch;
  incomingRequest.urlLength++;
  return 1;
 }
 // parse http version
 if(incomingRequestState == HTTP_REQUEST_STATE_VERSION){
  if(ch == '\r' || ch == '\n'){
   incomingRequest.version[incomingRequestPosition] = 0;
   incomingRequest.headersLenght = 0;
   if(ch == '\r'){
    incomingRequestState = HTTP_REQUEST_STATE_MAC_END_HEADER;
    return 1;
   }
   if(ch == '\n'){
    incomingRequestState = HTTP_REQUEST_STATE_LINUX_END_HEADER;
    return 1;
   }
  }
  if(incomingRequestPosition >= HTTP_MAX_VERSION_LENGTH){
   HttpSendResponseHeader(incomingRequestConnectionId, 431, "Version Too Long", "", 0, 0);
   return 0;
  }
  incomingRequest.version[incomingRequestPosition] = ch;
  incomingRequestPosition++;
  return 1;
 }
 // parse headers end
 if(incomingRequestState == HTTP_REQUEST_STATE_MAC_END_HEADER){
  if(ch == '\n'){
   incomingRequestState = HTTP_REQUEST_STATE_WIN_END_HEADER;
   return 1;
  }
  if(ch == '\r'){
   incomingRequestState = HTTP_REQUEST_STATE_END_HEADER;
   if(!HttpHeadersPutChar('\n')){
    return 0;
   }
   incomingRequest.dataLength = 0;
   incomingRequest.headers[incomingRequest.headersLenght] = 0;
   return 1;
  }
  incomingRequestState = HTTP_REQUEST_STATE_HEADER;
  if(!HttpHeadersPutChar(ch)){
   return 0;
  }
  return 1;
 }
 if(incomingRequestState == HTTP_REQUEST_STATE_LINUX_END_HEADER || incomingRequestState == HTTP_REQUEST_STATE_WIN_END_HEADER2){
  if(ch == '\n'){
   incomingRequestState = HTTP_REQUEST_STATE_END_HEADER;
   if(!HttpHeadersPutChar('\n')){
    return 0;
   }
   incomingRequest.dataLength = 0;
   incomingRequest.headers[incomingRequest.headersLenght] = 0;
   return 1;
  }
  incomingRequestState = HTTP_REQUEST_STATE_HEADER;
  if(!HttpHeadersPutChar(ch)){
   return 0;
  }
  return 1;
 }
 if(incomingRequestState == HTTP_REQUEST_STATE_WIN_END_HEADER){
  if(ch == '\r'){
   incomingRequestState = HTTP_REQUEST_STATE_WIN_END_HEADER2;
   return 1;
  }
  incomingRequestState = HTTP_REQUEST_STATE_HEADER;
  if(!HttpHeadersPutChar(ch)){
   return 0;
  }
  return 1;
 }
 // parse header rows
 if(incomingRequestState == HTTP_REQUEST_STATE_HEADER){
  if(ch == '\r'){
   incomingRequestState = HTTP_REQUEST_STATE_MAC_END_HEADER;
   if(!HttpHeadersPutChar('\n')){
    return 0;
   }
   return 1;
  }
  if(ch == '\n'){
   incomingRequestState = HTTP_REQUEST_STATE_LINUX_END_HEADER;
  }
  if(!HttpHeadersPutChar(ch)){
   return 0;
  }
  return 1;
 }
 HttpSendResponseHeader(incomingRequestConnectionId, 400, "Headers Syntax Error", "", 0, 0);
 return 0;
}

//*****************************************************************************************
//
// Function : HttpParseHeaderValue
// Description : function parse header value from request
// return HttpHeaderValue structure with null pointer value property if header not found or header row have wrong syntax
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
// Description : function send response for client, can be used only once per request
//
//*****************************************************************************************
unsigned char HttpSendResponse(unsigned short statusCode, unsigned char *statusMessage, unsigned char *headers, unsigned short headersLength, unsigned char *data, unsigned short dataLength){
 if(incomingRequestState != HTTP_REQUEST_STATE_END_REQUEST){
  return 0;
 }
 if(!HttpSendResponseHeader(incomingRequestConnectionId, statusCode, statusMessage, headers, headersLength, dataLength)){
  return 0;
 }
 if(dataLength == 0){
  return 1;
 }
 if(!TcpSendData(incomingRequestConnectionId, 60000, data, dataLength)){
  return 0;
 }
 return 1;
}

//*****************************************************************************************
//
// Function : TcpOnNewConnection
// Description : defined tcp callback for accept new incoming connection on http port
//
//*****************************************************************************************
unsigned char TcpOnNewConnection(const unsigned char connectionId){
 if(TcpGetConnection(connectionId)->port == HTTP_SERVER_PORT){
  if(incomingRequestState != HTTP_REQUEST_STATE_NO_REQUEST){
   return NET_HANDLE_RESULT_DROP;
  }
  incomingRequestConnectionId = connectionId;
  incomingRequestState = HTTP_REQUEST_STATE_START_REQUEST;
  incomingRequestPosition = 0;
  return NET_HANDLE_RESULT_OK;
 }
 return NET_HANDLE_RESULT_DROP;
}

//*****************************************************************************************
//
// Function : TcpOnNewConnection
// Description : defined tcp callback for detect new connection and prepare memory for request handling
//
//*****************************************************************************************
void TcpOnConnect(const unsigned char connectionId){
 if(TcpGetConnection(connectionId)->port == HTTP_SERVER_PORT){
  return;
 }
}

//*****************************************************************************************
//
// Function : TcpOnIncomingData
// Description : handle incoming http request by tcp callback
//
//*****************************************************************************************
void TcpOnIncomingData(const unsigned char connectionId, const unsigned char *data, unsigned short dataLength){
 if(TcpGetConnection(connectionId)->port != HTTP_SERVER_PORT){
  return;
 }
 if(incomingRequestState == HTTP_REQUEST_STATE_NO_REQUEST || connectionId != incomingRequestConnectionId){
  TcpDisconnect(connectionId, 5000);
  return;
 }
 // parse http headers
 unsigned short dataPosition;
 if(incomingRequestState < HTTP_REQUEST_STATE_END_HEADER){
  for(dataPosition=0; dataPosition<dataLength; dataPosition++){
   if(!HttpParseHeader(data[dataPosition])){
    TcpDisconnect(connectionId, 5000);
    return;
   }
   if(incomingRequestState == HTTP_REQUEST_STATE_END_HEADER){
    dataPosition++;
    break;
   }
  }
  if(incomingRequestState != HTTP_REQUEST_STATE_END_HEADER){
   return;
  }
 }
 // read request body
 if(strcmp(incomingRequest.method, "HEAD") == 0 || strcmp(incomingRequest.method, "GET") == 0){
  incomingRequestState = HTTP_REQUEST_STATE_END_REQUEST;
 }else{
  const HttpHeaderValue contentLength = HttpParseHeaderValue(&incomingRequest, "Content-Length");
  if(!contentLength.value){
   HttpSendResponseHeader(connectionId, 411, 0, "", 0, 0);
   TcpDisconnect(connectionId, 5000);
   return;
  }
  unsigned long httpDataLength;
  if(!ParseLong(&httpDataLength, contentLength.value, contentLength.length)){
   HttpSendResponseHeader(connectionId, 400, "Header Content-Length Syntax Error", "", 0, 0);
   TcpDisconnect(connectionId, 5000);
   return;
  }
  if(httpDataLength > HTTP_MAX_DATA_LENGTH){
   HttpSendResponseHeader(connectionId, 413, 0, "", 0, 0);
   TcpDisconnect(connectionId, 5000);
   return;
  }
  dataLength -= dataPosition;
  data += dataPosition;
  memcpy(incomingRequest.data + incomingRequest.dataLength, data, dataLength);
  incomingRequest.dataLength += dataLength;
  if(incomingRequest.dataLength == httpDataLength){
   incomingRequestState = HTTP_REQUEST_STATE_END_REQUEST;
  }
 }
 // handle request and send response
 if(incomingRequestState == HTTP_REQUEST_STATE_END_REQUEST){
  HttpOnIncomingRequest(&incomingRequest);
  if(incomingRequestState != HTTP_REQUEST_STATE_RESPONSE_HEADER_SENT){
   HttpSendResponseHeader(connectionId, 204, 0, "", 0, 0);
  }
  TcpDisconnect(connectionId, 5000);
 }
 return;
}

//*****************************************************************************************
//
// Function : TcpOnDisconnect
// Description : after handling request reset memory and be ready for another http request
//
//*****************************************************************************************
void TcpOnDisconnect(const unsigned char connectionId){
 if(TcpGetConnection(connectionId)->port == HTTP_SERVER_PORT){
  if(connectionId != incomingRequestConnectionId || incomingRequestState == HTTP_REQUEST_STATE_NO_REQUEST){
   return;
  }
  incomingRequestConnectionId = TCP_INVALID_CONNECTION_ID;
  incomingRequestState = HTTP_REQUEST_STATE_NO_REQUEST;
  return;
 }
}
