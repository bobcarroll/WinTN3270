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
#include "C3270Char.h"
#include "CEbcdicTranslator.h"

using namespace WinTN3270;
using namespace System;
using namespace System::Drawing;

/*********************************************************** 
 Creates a NULL character object.
***********************************************************/
C3270Char::C3270Char()
{
	this->__Constructor(NULL, false);
}

/***********************************************************
 Creates a character object from a byte.

 @param chChar the EBCDIC character to use
***********************************************************/
C3270Char::C3270Char(Byte chChar)
{
	this->__Constructor(chChar, false);
}

/***********************************************************
 Creates a character object from a byte. This is the 
 start-of-field byte.

 @param chChar the EBCDIC character to use
 @param fStartField flag to indicate start-of-field
***********************************************************/
C3270Char::C3270Char(Byte chChar, bool fStartField)
{
	this->__Constructor(chChar, true);
	this->UnserializeAttributes();
}

/***********************************************************
 Creates a character object from a byte. This is the real
 constructor.

 @param chChar the EBCDIC character to use
 @param fStartField flag to indicate start-of-field
***********************************************************/
void C3270Char::__Constructor(System::Byte chChar, bool fStartField)
{
	m_chChar = (fStartField ? chChar : CEbcdicTranslator::CharToASCII(chChar));
	m_chInitChar = m_chChar;
	m_fAutoSkip = false;
	m_fModified = false;
	m_fNumeric = false;
	m_fProtected = false;
	m_fStartField = fStartField;
	m_nDisplay = 0;
}

/***********************************************************
 Gets the appropriate color brush.

 @return the brush to paint with
***********************************************************/
Brush^ C3270Char::GetPaintBrush()
{
	switch (m_nDisplay) {
	
	/* Bright */
	case DisplayOptions::Bright:
		return Brushes::White;
	
	/* Dark */
	case DisplayOptions::Dark:
		return Brushes::Black;

	/* Normal */
	default:
		return Brushes::Green;
	}
}

/***********************************************************
 Gets the appropriate color brush.

 @param fBaseColor flag to enable base colors

 @return the brush to paint with
***********************************************************/
Brush^ C3270Char::GetPaintBrush(bool fBaseColor)
{
	bool fBright = (m_nDisplay == DisplayOptions::Bright);

	/* Get the base color */
	if (!m_fProtected && !fBright) /* Green */
		return Brushes::Green;
	else if (!m_fProtected && fBright) /* Red */
		return Brushes::Red;
	else if (m_fProtected && !fBright) /* Blue */
		return Brushes::Blue;
	else if (m_fProtected && fBright) /* White */
		return Brushes::White;
	
	return this->GetPaintBrush();
}

/***********************************************************
 Creates a string representation of the character object.

 @return a pretty string
***********************************************************/
String^ C3270Char::ToString()
{
	return this->ToString(false);
}

/***********************************************************
 Creates a string representation of the character object.

 @param fMasked flag to mask the character

 @return a pretty string
***********************************************************/
String^ C3270Char::ToString(bool fMasked)
{
	/* Null character displays nothing */
	if (m_chChar == 0 || m_fStartField)
		return " ";

	/* Some unprotected fields are dark so we mask the real character */
	if (fMasked && m_chChar != m_chInitChar)
		return "*";

	/* Otherwise output the character array */
	array<wchar_t>^ pchString = { m_chChar };
	return gcnew String(pchString);
}

/***********************************************************
 Reads the field attribute bit field and sets class vars
 accordingly.

 @param chAttr the attribute byte
***********************************************************/
void C3270Char::UnserializeAttributes()
{
	/* Bit 7: Modified Data Tag */
	m_fModified = (m_chChar & 0x01);

	/* Bit 6: Reserved, always zero */

	/* Bits 4-5: Field Display */
	if ((m_chChar >> 2 & 0x03) == 0x00)
		m_nDisplay = DisplayOptions::NormalNoLPDetect;
	else if ((m_chChar >> 2 & 0x03) == 0x01)
		m_nDisplay = DisplayOptions::NormalLPDetect;
	else if ((m_chChar >> 2 & 0x03) == 0x02)
		m_nDisplay = DisplayOptions::Bright;
	else if ((m_chChar >> 2 & 0x03) == 0x03)
		m_nDisplay = DisplayOptions::Dark;

	/* Bit 3: Numeric Only */
	m_fNumeric = (m_chChar >> 4 & 0x01);
	
	/* Bit 2: Protected */
	m_fProtected = (m_chChar >> 5 & 0x01);
	if (m_fNumeric && m_fProtected)
		m_fAutoSkip = true;

	/* Bit 1: Unknown (Ignored) */

	/* Bit 0: Unknown (Ignored) */
}
