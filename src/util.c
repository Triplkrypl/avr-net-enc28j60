inline unsigned char low(unsigned short value){
 return (value&0xFF);
}

inline unsigned char high(unsigned short value){
 return ((value>>8)&0xFF);
}
