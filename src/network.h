#ifndef NETWORK
#define NETWORK
void NetInit();
void NetHandleNetwork();
void NetHandleIncomingPacket(unsigned char *buffer, unsigned short length);
#endif
