#ifndef NETWORK

#define NET_MIN_DINAMIC_PORT 49152
#define NET_MAX_PORT 65535

#define NETWORK
void NetInit();
void NetHandleNetwork();
void NetHandleIncomingPacket(unsigned short length);
unsigned char *NetGetBuffer();
#endif
