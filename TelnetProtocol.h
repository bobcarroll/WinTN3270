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
 Constants and data structures for TELNET.
***********************************************************/

#pragma once

/* Commands */
#define TELNET_COMMAND_END_OF_RECORD	0xEF
#define TELNET_COMMAND_SE				0xF0
#define TELNET_COMMAND_NOP				0xF1
#define TELNET_COMMAND_DATA_MARK		0xF2
#define TELNET_COMMAND_BREAK			0xF3
#define TELNET_COMMAND_INTERRUPT		0xF4
#define TELNET_COMMAND_ABORT			0xF5
#define TELNET_COMMAND_ARE_YOU_THERE	0xF6
#define TELNET_COMMAND_ERASE_CHAR		0xF7
#define TELNET_COMMAND_ERASE_LINE		0xF8
#define TELNET_COMMAND_GA				0xF9
#define TELNET_COMMAND_SB				0xFA
#define TELNET_COMMAND_WILL				0xFB
#define TELNET_COMMAND_WONT				0xFC
#define TELNET_COMMAND_DO				0xFD
#define TELNET_COMMAND_DONT				0xFE
#define TELNET_COMMAND_IAC				0xFF

/* Options */
#define TELNET_OPTION_BINARY			0x00
#define TELNET_OPTION_ECHO				0x01
#define TELNET_OPTION_SUPPRESS_GA		0x03
#define TELNET_OPTION_TERM_TYPE			0x18
#define TELNET_OPTION_END_OF_RECORD		0x19
#define TELNET_OPTION_TN3270E			0x28

/* Sub-Option Structure */
ref struct TNSUBOPTION
{
	cli::array<System::Byte>^ pchValue;
	bool fValueRequired;
	int nOptionId;
};
