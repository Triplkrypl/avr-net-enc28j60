#ifndef UTIL
#define UTIL
inline unsigned char Low(unsigned short value){
 return (value&0xFF);
}

inline unsigned char High(unsigned short value){
 return ((value>>8)&0xFF);
}

inline unsigned short CharsToShort(const unsigned char *value){
 unsigned short numericValue;
 ((unsigned char*)&numericValue)[1] = value[0];
 ((unsigned char*)&numericValue)[0] = value[1];
 return numericValue;
}

inline void CharsPutShort(unsigned char *chars, const unsigned short value){
 chars[0] = ((unsigned char*)&value)[1];
 chars[1] = ((unsigned char*)&value)[0];
}

inline unsigned long CharsToLong(const unsigned char *value){
 unsigned long numericValue;
 ((unsigned char*)&numericValue)[3] = value[0];
 ((unsigned char*)&numericValue)[2] = value[1];
 ((unsigned char*)&numericValue)[1] = value[2];
 ((unsigned char*)&numericValue)[0] = value[3];
 return numericValue;
}
#endif
