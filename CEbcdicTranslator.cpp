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

#include "stdafx.h"
#include "CEbcdicTranslator.h"

using namespace WinTN3270;
using namespace System;

/***********************************************************
 Translates a byte from EBCDIC to ASCII.

 @param mpInput the input byte array

 @return an ASCII-encoded character array
***********************************************************/
wchar_t CEbcdicTranslator::CharToASCII(Byte chInput)
{
	return (wchar_t)m_mpEbc2AscTbl[(int) chInput];
}

/***********************************************************
 Translates a byte from ASCII to EBCDIC.

 @param pchInput the input character array

 @return an EBCDIC-encoded byte array
***********************************************************/
Byte CEbcdicTranslator::CharToEBCDIC(wchar_t chInput)
{
	return (Byte)m_mpAsc2EbcTbl[(int) chInput];
}

/***********************************************************
 Translates bytes from EBCDIC to ASCII.

 @param mpInput the input byte array

 @return an ASCII-encoded character array
***********************************************************/
array<wchar_t>^ CEbcdicTranslator::StringToASCII(array<Byte>^ mpInput)
{
	array<wchar_t>^ pchOutput = gcnew array<wchar_t>(mpInput->Length);

	/* Convert characters using the EBCDIC-to-ASCII table */
	for (int i = 0; i < mpInput->Length; i++)
		pchOutput[i] = CEbcdicTranslator::CharToASCII(mpInput[i]);

	return pchOutput;
}

/***********************************************************
 Translates bytes from ASCII to EBCDIC.

 @param pchInput the input character array

 @return an EBCDIC-encoded byte array
***********************************************************/
array<Byte>^ CEbcdicTranslator::StringToEBCDIC(array<wchar_t>^ pchInput)
{
	array<Byte>^ mpOutput = gcnew array<Byte>(pchInput->Length);

	/* Convert characters using the ASCII-to-EBCDIC table */
	for (int i = 0; i < pchInput->Length; i++)
		mpOutput[i] = CEbcdicTranslator::CharToEBCDIC(pchInput[i]);

	return mpOutput;
}
