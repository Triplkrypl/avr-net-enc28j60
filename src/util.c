inline unsigned char Low(unsigned short value){
 return (value&0xFF);
}

inline unsigned char High(unsigned short value){
 return ((value>>8)&0xFF);
}

inline unsigned short CharsToShort(const char *value){
 unsigned short numericValue;
 ((unsigned char*)&numericValue)[1] = value[0];
 ((unsigned char*)&numericValue)[0] = value[1];
 return numericValue;
}
