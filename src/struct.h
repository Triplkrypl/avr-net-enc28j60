#ifndef STRUCT
#define STRUCT
//********************************************************************************************
//
// File : _struct.h header for all structure
//
//********************************************************************************************
//
// Copyright (C) 2007
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
// This program is distributed in the hope that it will be useful, but
//
// WITHOUT ANY WARRANTY;
//
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin St, Fifth Floor, Boston, MA 02110, USA
//
// http://www.gnu.de/gpl-ger.html
//
//********************************************************************************************
typedef unsigned char	BYTE;				// 8-bit
typedef unsigned int	WORD;				// 16-bit
typedef unsigned long	DWORD;				// 32-bit

typedef union _BYTE_BITS
{
    BYTE byte;
    struct
    {
        unsigned char bit0:1;
        unsigned char bit1:1;
        unsigned char bit2:1;
        unsigned char bit3:1;
        unsigned char bit4:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
    } bits;
} BYTE_BITS;

typedef union _WORD_BYTES
{
    WORD word;
    BYTE bytes[2];
    struct
    {
        BYTE low;
        BYTE high;
    } byte;
    struct
    {
        unsigned char bit0:1;
        unsigned char bit1:1;
        unsigned char bit2:1;
        unsigned char bit3:1;
        unsigned char bit4:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
        unsigned char bit8:1;
        unsigned char bit9:1;
        unsigned char bit10:1;
        unsigned char bit11:1;
        unsigned char bit12:1;
        unsigned char bit13:1;
        unsigned char bit14:1;
        unsigned char bit15:1;
    } bits;
} WORD_BYTES;

typedef union _DWORD_BYTE
{
    DWORD dword;
	WORD words[2];
    BYTE bytes[4];
    struct
    {
        WORD low;
        WORD high;
    } word;
    struct
    {
        BYTE LB;
        BYTE HB;
        BYTE UB;
        BYTE MB;
    } byte;
    struct
    {
        unsigned char bit0:1;
        unsigned char bit1:1;
        unsigned char bit2:1;
        unsigned char bit3:1;
        unsigned char bit4:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
        unsigned char bit8:1;
        unsigned char bit9:1;
        unsigned char bit10:1;
        unsigned char bit11:1;
        unsigned char bit12:1;
        unsigned char bit13:1;
        unsigned char bit14:1;
        unsigned char bit15:1;
        unsigned char bit16:1;
        unsigned char bit17:1;
        unsigned char bit18:1;
        unsigned char bit19:1;
        unsigned char bit20:1;
        unsigned char bit21:1;
        unsigned char bit22:1;
        unsigned char bit23:1;
        unsigned char bit24:1;
        unsigned char bit25:1;
        unsigned char bit26:1;
        unsigned char bit27:1;
        unsigned char bit28:1;
        unsigned char bit29:1;
        unsigned char bit30:1;
        unsigned char bit31:1;
    } bits;
} DWORD_BYTES;

// ethernet header structure
typedef struct _ETH_HEADER
{
	BYTE	dest_mac[6];
	BYTE	src_mac[6];
	WORD_BYTES	type;
}ETH_HEADER;

// IP header structure
typedef struct _IP_HEADER
{
	BYTE		version_hlen;
	BYTE		type_of_service;
	WORD_BYTES	total_length;
	WORD_BYTES	indentificaton;
	WORD_BYTES	flagment;
	BYTE		time_to_live;
	BYTE		protocol;
	WORD_BYTES	checksum;
	BYTE		src_ip[4];
	BYTE		dest_ip[4];

}IP_HEADER;

// ICMP packet structure
#define ICMP_MAX_DATA	60
typedef struct _ICMP_HEADER
{
	BYTE		type;
	BYTE		code;
	WORD_BYTES	checksum;
	WORD_BYTES	identifier;
	WORD_BYTES	sequence_number;
} ICMP_HEADER;

#endif
