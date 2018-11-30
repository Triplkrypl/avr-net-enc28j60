#ifndef UDP
#define UDP
//********************************************************************************************
//
// File : udp.h implement for User Datagram Protocol
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

#define UDP_SRC_PORT_H_P	0x22
#define UDP_SRC_PORT_L_P	0x23
#define UDP_DST_PORT_H_P	0x24
#define UDP_DST_PORT_L_P	0x25
#define UDP_LENGTH_H_P		0x26
#define UDP_LENGTH_L_P		0x27
#define UDP_CHECKSUM_H_P	0x28
#define UDP_CHECKSUM_L_P	0x29
#define UDP_DATA_P			0x2A

extern WORD udp_puts_data ( BYTE *rxtx_buffer, BYTE *data, WORD offset, unsigned short length );
extern BYTE udp_is_udp ( BYTE *rxtx_buffer, unsigned short dest_port, unsigned short *source_port);
extern void udp_send(unsigned char* rxtx_buffer, unsigned short source_port, unsigned char* dest_mac, unsigned char *dest_ip, unsigned short dest_port,  unsigned short data_length);
#endif
