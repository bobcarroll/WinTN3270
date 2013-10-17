/***********************************************************
 WinTN3270
 Copyright © 2007 Bob Carroll (bob.carroll@alum.rit.edu)
 
 This software is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public 
 License as published by the Free Software Foundation; 
 either version 2, or (at your option) any later version.

 This software is distributed in the hope that it will be 
 useful, but WITHOUT ANY WARRANTY; without even the implied 
 warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 PURPOSE.  See the GNU General Public License for more 
 details.

 You should have received a copy of the GNU General Public 
 License along with this software; if not, write to the 
 Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, 
 Boston, MA  02110-1301 USA
***********************************************************/

/***********************************************************
 Constants for the IBM 3270 data stream protocol.
***********************************************************/

#pragma once

/* Commands (ASCII) */
#define IBM3270DS_CMD_EAU			0x0F	/* Erase All Unprotected */
#define IBM3270DS_CMD_EW			0x05	/* Erase Write */
#define IBM3270DS_CMD_EWA			0x0D	/* Erase Write Alternate */
#define IBM3270DS_CMD_NOP			0x03	/* No Operation */
#define IBM3270DS_CMD_RB			0x02	/* Read Buffer */
#define IBM3270DS_CMD_RM			0x06	/* Read Modified */
#define IBM3270DS_CMD_RMA			0x0E	/* Read Modified All */
#define IBM3270DS_CMD_W				0x01	/* Write */
#define IBM3270DS_CMD_WSF			0x11	/* Write Structured Field */

/* Commands (EBCDIC) */
#define IBM3270DS_CMD_EAU_EBCDIC	0x6F	/* Erase All Unprotected */
#define IBM3270DS_CMD_EW_EBCDIC		0xF5	/* Erase Write */
#define IBM3270DS_CMD_EWA_EBCDIC	0x7E	/* Erase Write Alternate */
#define IBM3270DS_CMD_RB_EBCDIC		0xF2	/* Read Buffer */
#define IBM3270DS_CMD_RM_EBCDIC		0xF6	/* Read Modified */
#define IBM3270DS_CMD_RMA_EBCDIC	0x6E	/* Read Modified All */
#define IBM3270DS_CMD_W_EBCDIC		0xF1	/* Write */
#define IBM3270DS_CMD_WSF_EBCDIC	0xF3	/* Write Structured Field */

/* Orders */
#define IBM3270DS_ORDER_EUA			0x12	/* Erase Unprotected to Address */
#define IBM3270DS_ORDER_GE			0x08	/* Graphic Escape */
#define IBM3270DS_ORDER_IC			0x13	/* Insert Cursor */
#define IBM3270DS_ORDER_MF			0x2C	/* Modify Field */
#define IBM3270DS_ORDER_PT			0x05	/* Program Tab */
#define IBM3270DS_ORDER_RA			0x3C	/* Repeat to Address */
#define IBM3270DS_ORDER_SA			0x28	/* Set Attribute */
#define IBM3270DS_ORDER_SBA			0x11	/* Set Buffer Address */
#define IBM3270DS_ORDER_SF			0x1D	/* Start Field */
#define IBM3270DS_ORDER_SFE			0x29	/* Start Field Extended */

/* Action IDs */
#define IBM3270DS_AID_CLEAR			0x6D
#define IBM3270DS_AID_ENTER			0x7D
#define IBM3270DS_AID_NONE			0x60
#define IBM3270DS_AID_PA1			0x6C
#define IBM3270DS_AID_PA2			0x6E
#define IBM3270DS_AID_PA3			0x6B
#define IBM3270DS_AID_PF1			0xF1
#define IBM3270DS_AID_PF2			0xF2
#define IBM3270DS_AID_PF3			0xF3
#define IBM3270DS_AID_PF4			0xF4
#define IBM3270DS_AID_PF5			0xF5
#define IBM3270DS_AID_PF6			0xF6
#define IBM3270DS_AID_PF7			0xF7
#define IBM3270DS_AID_PF8			0xF8
#define IBM3270DS_AID_PF9			0xF9
#define IBM3270DS_AID_PF10			0x7A
#define IBM3270DS_AID_PF11			0x7B
#define IBM3270DS_AID_PF12			0x7C
#define IBM3270DS_AID_PF13			0xC1
#define IBM3270DS_AID_PF14			0xC2
#define IBM3270DS_AID_PF15			0xC3
#define IBM3270DS_AID_PF16			0xC4
#define IBM3270DS_AID_PF17			0xC5
#define IBM3270DS_AID_PF18			0xC6
#define IBM3270DS_AID_PF19			0xC7
#define IBM3270DS_AID_PF20			0xC8
#define IBM3270DS_AID_PF21			0xC9
#define IBM3270DS_AID_PF22			0x4A
#define IBM3270DS_AID_PF23			0x4B
#define IBM3270DS_AID_PF24			0x4C
#define IBM3270DS_AID_SF			0x88

/* Structured Field Operations */
#define IBM3270DS_SF_ERASE_RESET	0x03	/* Erase/Reset */
#define IBM3270DS_SF_OUTBOUND_DS	0x40	/* Outbound 3270 DS */
#define IBM3270DS_SF_QUERY_REPLY	0x81	/* Query Reply */
#define IBM3270DS_SF_READ_PART		0x01	/* Read Partition */
#define IBM3270DS_SF_RESET_PART		0x00	/* Reset Partition */
#define IBM3270DS_SF_SET_REPLY_MODE	0x09	/* Set Reply Mode */
#define IBM3270DS_SF_TRANSFER_DATA	0xd0	/* File Transfer Open Request */

/* SF Erase/Write Operations */
#define IBM3270DS_SF_ER_DEFAULT		0x00	/*  Default */
#define IBM3270DS_SF_ER_ALT			0x80	/*  Alternate */

/* SF Read Partition Operations */
#define IBM3270DS_SF_RP_QUERY		0x02	/* Query */
#define IBM3270DS_SF_RP_QLIST		0x03	/* Query List */
#define IBM3270DS_SF_RP_RB			0xF2	/* Read Buffer */
#define IBM3270DS_SF_RP_RM			0xF6	/* Read Modified */
#define IBM3270DS_SF_RP_RMA			0x6E	/* Read Modified All */

/* SF Set Reply Mode Operations */
#define IBM3270DS_SF_SRM_FIELD		0x00	/* Field */
#define IBM3270DS_SF_SRM_XFIELD		0x01	/* Extended Field */
#define IBM3270DS_SF_SRM_CHAR		0x02	/* Character */

/* SF Query Reply Codes */
#define IBM3270DS_SF_QR_NULL		0xFF	/* Null */
