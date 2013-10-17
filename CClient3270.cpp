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
#include "CClient3270.h"
#include "IBM3270.h"
#include "C3270Char.h"
#include "CEbcdicTranslator.h"
#include "stdio.h"

using namespace WinTN3270;
using namespace System;
using namespace System::Drawing;
using namespace System::Windows::Forms;

/***********************************************************
 Creates a new TN3270 client instance.

 @param cchIPAddr the IP address of the remote host
 @param nPort the remote TCP port
***********************************************************/
CClient3270::CClient3270(String^ cchIPAddr, int nPort)
{
	this->__Constructor(cchIPAddr, nPort, 0);
}

/***********************************************************
 Creates a new TN3270 client instance.

 @param cchIPAddr the IP address of the remote host
 @param nPort the remote TCP port
 @param nModel the terminal model number
***********************************************************/
CClient3270::CClient3270(String^ cchIPAddr, int nPort, int nModel)
{
	this->__Constructor(cchIPAddr, nPort, nModel);
}

/***********************************************************
 Disconnects from the remote host.
***********************************************************/
CClient3270::~CClient3270()
{
	this->Disconnect();
}

/***********************************************************
 Creates a new TN3270 client instance. Unlike the above two
 functions, this is the real constructor.

 @param cchIPAddr the IP address of the remote host
 @param nPort the remote TCP port
 @param nModel the terminal model number
***********************************************************/
void CClient3270::__Constructor(System::String^ cchIPAddr, int nPort, int nModel)
{
	m_mpClient = gcnew CTelnetClient(cchIPAddr, nPort);

	/* Set the screen dimensions */
	switch (nModel) {

	/* 24x80 */
	case 2:
		m_nRows = 24;
		m_nCols = 80;
		break;

	/* 32x80 */
	case 3:
		m_nRows = 32;
		m_nCols = 80;
		break;

	/* 43x80 */
	case 4:
		m_nRows = 43;
		m_nCols = 80;
		break;

	/* 27x132 */
	case 5:
		m_nRows = 27;
		m_nCols = 132;
		break;

	/* Default to model 2 */
	default:
		nModel = 2;
		m_nRows = 24;
		m_nCols = 80;
	}

	/* Set the model and term type */
	char* pchModel = (char*)malloc(sizeof(char) + 1);
	sprintf_s(pchModel, sizeof(char) + 1, "%d", nModel);
	array<Byte>^ mpTermType = 
		{ 'I', 'B', 'M', '-', '3', '2', '7', '8', '-', *pchModel, '-', 'E' };
	free(pchModel);
	m_nModel = nModel;

	/* Set default 3270 options */
	m_mpClient->TerminalType = mpTermType;
	m_mpClient->BinaryTransmission = true;
	m_mpClient->EndOfRecord = true;

	/* Register callbacks */
	m_mpClient->ReceiveCallbackBinary =
		gcnew CTelnetClient::OnReceiveBinary(this, &CClient3270::ReceiveCallback);
	m_mpClient->SendCallback = 
		gcnew CTelnetClient::OnSend(this, &CClient3270::SendCallback);

	m_mpCharBuffer = 
		gcnew array<C3270Char^>(this->Size.Height * this->Size.Width);
	m_mpOnPaintCbk = nullptr;
	m_fBaseColor = true;
	m_fKeybLocked = true;
	m_fSoundAlaram = false;
	m_fStartPrinter = false;
	m_nBufferPos = 0;
	m_nCursorPos = 0;
	m_nActionId = 0;
	m_nLineLength = 0;
	m_nLastField = 0;

	this->EraseScreen();
}

/***********************************************************
 Opens a new connection to the remote host.
***********************************************************/
void CClient3270::Connect()
{
	this->Connect(false);
}

/***********************************************************
 Opens a new connection to the remote host.

 @param fSecure enable SSL
***********************************************************/
void CClient3270::Connect(bool fSecure)
{
	m_mpClient->Connect(fSecure);
}

/***********************************************************
 Converts the 6-bit buffer address to an integer.

 @param chFirst the first byte of the address
 @param chSecond the second byte of the address

 @return the buffer offset as an integer
***********************************************************/
int CClient3270::ConvertBufferAddress(Byte chFirst, Byte chSecond)
{
	/* Do some bit-shifting voodoo to get the offset */
	return ((chFirst & 0xC0) == 0x00 ?
		((chFirst & 0x3F) << 8) + chSecond :
		((chFirst & 0x3F) << 6) + (chSecond & 0x3F));
}

/***********************************************************
 Creates a key entry action for transmission to the
 mainframe.

 @param chActionId the action id byte

 @return an action array
***********************************************************/
array<Byte>^ CClient3270::CreateActionArray(Byte chActionId)
{
	array<Byte>^ mpAction = gcnew array<Byte>(3);
	mpAction[0] = chActionId;
	mpAction[1] = m_mpAddrTbl[(m_nCursorPos >> 6) & 0x3F];
	mpAction[2] = m_mpAddrTbl[m_nCursorPos & 0x3F];

	return mpAction;
}

/***********************************************************
 Terminates the active stream connection.
***********************************************************/
void CClient3270::Disconnect()
{
	m_mpClient->Disconnect();
}

/***********************************************************
 Performs a terminal input action.

 @param chKeyChar the key character

 @return TRUE for success, FALSE otherwise
***********************************************************/
bool CClient3270::DoInputAction(wchar_t chKeyChar)
{
	array<Byte>^ mpResp;

	switch (chKeyChar) {

	/* Enter */
	case ConsoleKey::Enter:
		/* Send the action */
		mpResp = this->CreateActionArray(IBM3270DS_AID_ENTER);
		mpResp = this->MergeModifiedFields(mpResp);
		m_mpClient->Send(mpResp);
		return true;

	/* Clear Screen */
	case ConsoleKey::F12:
		/* Send the action */
		mpResp = gcnew array<Byte>(1);
		mpResp[0] = IBM3270DS_AID_CLEAR;
		m_mpClient->Send(mpResp);
	}

	return false;
}

/***********************************************************
 Resets all unprotected fields and jumps to the first.
***********************************************************/
void CClient3270::EraseAllUnprotected()
{
	bool fInField = false;

	for (int i = 0; i < m_mpCharBuffer->Length; i++) {
		/* Look for the start of field character */
		if (m_mpCharBuffer[i]->StartField) {
			/* Reset the MDT of unprotected fields */
			fInField = !m_mpCharBuffer[i]->Protected;
			if (fInField) 
				m_mpCharBuffer[i]->Modified = false;
			continue;
		}

		if (!fInField)
			continue;

		/* Null out the character */
		m_mpCharBuffer[i]->Character = 0;
	}
}

/***********************************************************
 Clears the character buffer.
***********************************************************/
void CClient3270::EraseScreen()
{
	for (int i = 0; i < m_mpCharBuffer->Length; i++)
		m_mpCharBuffer[i] = gcnew C3270Char();

	m_nBufferPos = 0;
	m_nCursorPos = 0;

	this->RepaintScreen();
}

/***********************************************************
 Finds the field start byte from a given offset.

 @param nOffset the starting offset position

 @return the start-of-field offset, or -1 if it can't find
 the SOF byte
***********************************************************/
int CClient3270::FindStartOfField(int nOffset)
{
	for (int i = nOffset; i >= 0; i--) {
		if (m_mpCharBuffer[i]->StartField)
			return i;
	}

	return -1;
}

/***********************************************************
 Reads the order byte and any attribute bytes and then
 changes the client state accordingly.

 @param mpData the incoming data buffer
 @param nOffset the starting offset position

 @return the new starting offset
***********************************************************/
int CClient3270::InterpretOrder(array<Byte>^ mpData, int nOffset)
{
	int nBuffAddr;

	/* Bounds check */
	if (mpData == nullptr || mpData->Length == 0 || nOffset >= mpData->Length)
		return nOffset;

	while (true) {
		switch (mpData[nOffset]) {

		/* Start Field */
		case IBM3270DS_ORDER_SF:
			/* The first byte is a field attribute */
			m_mpCharBuffer[m_nBufferPos] = gcnew C3270Char(mpData[nOffset + 1], true);
			m_nLastField = m_nBufferPos;
			m_nBufferPos++;

			/* Skip to the data */
			nOffset += 2;
			break;

		/* Start Field (Extended) */
		case IBM3270DS_ORDER_SFE:
			/* Not implemented */
			nOffset += 3;
			break;

		/* Set Buffer Address */
		case IBM3270DS_ORDER_SBA:
			m_nBufferPos = this->ConvertBufferAddress( 
				mpData[nOffset + 1], 
				mpData[nOffset + 2]
			);
			nOffset += 3;

			break;

		/* Insert Cursor */
		case IBM3270DS_ORDER_IC:
			/* Set the visible cursor position */
			m_nCursorPos = m_nBufferPos;
			m_nBufferPos++;
			nOffset++;
			break;

		/* Erase Unprotected Fields */
		case IBM3270DS_ORDER_EUA:
			this->EraseAllUnprotected();

			/* Jump to the first unprotected field */
			m_nCursorPos = 0;
			this->JumpToNextUnprotectedField();
			nOffset += 3;
			break;

		/* Modify Field */
		case IBM3270DS_ORDER_MF:
			/* Not implemented */
			nOffset += 6;
			break;

		/* Repeat to Address */
		case IBM3270DS_ORDER_RA:
			nBuffAddr = this->ConvertBufferAddress( 
				mpData[nOffset + 1], 
				mpData[nOffset + 2]
			);
			this->RepeatToAddress(mpData[nOffset + 3], nBuffAddr);
			nOffset += 4;
			break;

		/* Set Attribute */
		case IBM3270DS_ORDER_SA:
			/* Not implemented */
			nOffset += 3;
			break;

		/* Program Tab */
		case IBM3270DS_ORDER_PT:
			this->JumpToNextUnprotectedField();
			nOffset++;
			break;

		/* Graphic Escape */
		case IBM3270DS_ORDER_GE:
			/* Not implemented */
			nOffset += 2;
			break;

		/* Not an order byte */
		default:
			return nOffset;
		}

		if (nOffset >= mpData->Length) 
			return nOffset;
	}
}

/***********************************************************
 Determines if the given field is protected.

 @param nOffset the offset position in the buffer

 @return TRUE if protected, FALSE otherwise
***********************************************************/
bool CClient3270::IsProtectedField(int nOffset)
{
	/* If we're at the start of the field... */
	if (m_mpCharBuffer[nOffset]->StartField)
		return m_mpCharBuffer[nOffset]->Protected;

	/* ... we're not, so find the SOF first */
	int nFieldStart = this->FindStartOfField(nOffset);
	if (nFieldStart == -1)
		return false;

	return m_mpCharBuffer[nFieldStart]->Protected;
}

/***********************************************************
 Moves the cursor to the next unprotected field.
***********************************************************/
void CClient3270::JumpToNextUnprotectedField()
{
	bool fWrap = false;

	for (int i = m_nCursorPos + 1; i <= m_mpCharBuffer->Length; i++) {
		/* Wrap to the front if we're past the end */
		if (i == m_mpCharBuffer->Length) {
			fWrap = true;
			i = 0;
		}

		/* We're looking for start of fields */
		if (fWrap && i >= m_nCursorPos)
			break;
		if (!m_mpCharBuffer[i]->StartField)
			continue;

		/* Skip protected fields */
		if (m_mpCharBuffer[i]->Protected || m_mpCharBuffer[i]->AutoSkip)
			continue;

		this->MoveCursor(i - m_nCursorPos + 1);
		break;
	}
}

/***********************************************************
 Moves the cursor to the previous unprotected field.
***********************************************************/
void CClient3270::JumpToPrevUnprotectedField()
{
	bool fWrap = false;

	for (int i = m_nCursorPos - 2; i >= -1; i--) {
		/* Wrap to the end if we're past the front */
		if (i == -1) {
			fWrap = true;
			i = m_mpCharBuffer->Length -1;
		}

		/* We're looking for start of fields */
		if (fWrap && i <= m_nCursorPos)
			break;
		if (!m_mpCharBuffer[i]->StartField)
			continue;

		/* Skip protected fields */
		if (m_mpCharBuffer[i]->Protected || m_mpCharBuffer[i]->AutoSkip)
			continue;

		this->MoveCursor(-1 * (m_nCursorPos - i) + 1);
		break;
	}
}

/***********************************************************
 Simulates a key press on the terminal.

 @param mpArgs the keyboard event arguments
***********************************************************/
void CClient3270::KeyPress(KeyPressEventArgs^ mpArgs)
{
	this->KeyPress(mpArgs, Keys::None);
}

/***********************************************************
 Simulates a key press on the terminal.

 @param mpArgs the keyboard event arguments
 @param nModifiers bitwise combination of modifier keys
***********************************************************/
void CClient3270::KeyPress(KeyPressEventArgs^ mpArgs, Keys nModifiers)
{
	C3270Char^ mpChar = m_mpCharBuffer[m_nCursorPos];
	int nFieldStart;

	/* Get the previous cursor position */
	int nCursPrev = (m_nCursorPos == 0 ? m_mpCharBuffer->Length - 1 : m_nCursorPos - 1);

	/* Handle input actions */
	if (DoInputAction(mpArgs->KeyChar))
		return;
	
	/* Handle Tab */
	if (mpArgs->KeyChar == (wchar_t)ConsoleKey::Tab) {
		if (nModifiers == Keys::Shift)
			return this->JumpToPrevUnprotectedField();
		else
			return this->JumpToNextUnprotectedField();
	}

	/* Handle backspace */
	if (mpArgs->KeyChar == (wchar_t)ConsoleKey::Backspace
		&& !this->IsProtectedField(nCursPrev)
		&& !(nCursPrev < m_nCursorPos && m_mpCharBuffer[nCursPrev]->StartField))
	{
		/* Erase the last character */
		m_mpCharBuffer[nCursPrev]->Character = 0;
		nFieldStart = this->FindStartOfField(nCursPrev);
		if (nFieldStart >= 0)
			m_mpCharBuffer[nFieldStart]->Modified = true;
		return this->MoveCursor(-1);
	}

	/* Ignore non-characters */
	if (((int)mpArgs->KeyChar) < 32 || ((int)mpArgs->KeyChar) > 126)
		return;

	/* Check for the numeric-only flag */
	nFieldStart = this->FindStartOfField(m_nCursorPos);
	if (nFieldStart > -1 && m_mpCharBuffer[nFieldStart]->NumericOnly && 
			(((int)mpArgs->KeyChar) < 48 || ((int)mpArgs->KeyChar) > 57))
		return;

	/* Everything else */
	if (!mpChar->StartField && !this->IsProtectedField(m_nCursorPos)) {
		/* Write the character to the buffer */
		m_mpCharBuffer[m_nCursorPos]->Character = (wchar_t)mpArgs->KeyChar;
		if (nFieldStart != -1) 
			m_mpCharBuffer[nFieldStart]->Modified = true;
		this->MoveCursor(1);
	}
}

/***********************************************************
 Builds an array of modified fields for transmission to
 the mainframe.

 @param mpData the input data array
***********************************************************/
array<Byte>^ CClient3270::MergeModifiedFields(array<Byte>^ mpData)
{
	bool fInField = false;
	bool fCopy = false;

	for (int i = 0; i < m_mpCharBuffer->Length; i++) {
		/* Mark when we hit a field */
		if (m_mpCharBuffer[i]->StartField) 
			fInField = true;

		/* Look for modified fields */
		if (m_mpCharBuffer[i]->StartField && m_mpCharBuffer[i]->Modified) {
			array<Byte>::Resize(mpData, mpData->Length + 3);
			mpData[mpData->Length - 3] = IBM3270DS_ORDER_SBA;
			mpData[mpData->Length - 2] = m_mpAddrTbl[((i + 1) >> 6) & 0x3F];
			mpData[mpData->Length - 1] = m_mpAddrTbl[(i + 1) & 0x3F];
			fCopy = true;

			if (fInField)
				continue;
		}

		/* If we're not in a field, then copy the character, otherwise
		   only copy when the copy flag is set. */
		if (fInField && !fCopy)
			continue;

		/* Skip null characters */
		if (m_mpCharBuffer[i]->Character == NULL) {
			fCopy = false;
			continue;
		}

		array<Byte>::Resize(mpData, mpData->Length + 1);
		mpData[mpData->Length - 1] = 
			CEbcdicTranslator::CharToEBCDIC(m_mpCharBuffer[i]->Character);
	}

	return mpData;
}

/***********************************************************
 Sets the cursor position.

 @param nStep the increment value
***********************************************************/
void CClient3270::MoveCursor(int nStep)
{
	/* Move the cursor */
	int nNewPos = m_nCursorPos + nStep;
	if (nNewPos < 0) 
		nNewPos = m_mpCharBuffer->Length - 1;
	if (nNewPos >= m_mpCharBuffer->Length)
		nNewPos = 0;

	/* Find the coordinates */
	int nRow = (int)Math::Floor(nNewPos / this->Size.Width);
	int nCol = nNewPos - (nRow * this->Size.Width);

	/* Paint the cursor */
	this->MoveCursor(nCol, nRow);
}

/***********************************************************
 Sets the cursor position.

 @param x the x coordinate (abscissa)
 @param y the y coordinate (ordinate)
***********************************************************/
void CClient3270::MoveCursor(int x, int y)
{
	this->MoveCursor(x, y, false);
}

/***********************************************************
 Sets the cursor position.

 @param x the x coordinate (abscissa)
 @param y the y coordinate (ordinate)
 @param fRelative the flag to indicate relative position
***********************************************************/
void CClient3270::MoveCursor(int x, int y, bool fRelative)
{
	if (fRelative) {
		int nRow = (int)Math::Floor( m_nCursorPos / this->Size.Width);
		int nCol = m_nCursorPos - (nRow * this->Size.Width);

		x += nCol;
		if (x < 0)
			x += this->Size.Width;
		if (x >= this->Size.Width)
			x -= this->Size.Width;

		y += nRow;
		if (y < 0)
			y += this->Size.Height;
		if (y >= this->Size.Height)
			y -= this->Size.Height;
	}

	int nOldPos = m_nCursorPos;
	m_nCursorPos = (y * this->Size.Width) + x;

	this->RepaintScreen(nOldPos, nOldPos, false);
	this->RepaintScreen(m_nCursorPos, m_nCursorPos, false);
}

/***********************************************************
 Interprets the 3270 data stream.

 @param mpData -- the input buffer
 @param nLength -- the length of the stream
***********************************************************/
void CClient3270::ParseStream(array<Byte>^ mpData, int nLength)
{
	/* Is there anything to do? */
	if (mpData->Length == 0)
		return;

	/* Process the 3270 command */
	switch (mpData[0]) {

	/* Erase/Write */
	case IBM3270DS_CMD_EW:
	case IBM3270DS_CMD_EW_EBCDIC:
		this->EraseScreen();

	/* Write */
	case IBM3270DS_CMD_W:
	case IBM3270DS_CMD_W_EBCDIC:
		if (!this->UnserializeWCC(mpData, 1))
			break;
		this->WriteBuffer(mpData, 2, mpData->Length - 2);

		break;

	/* Write Structured Field */
	case IBM3270DS_CMD_WSF:
	case IBM3270DS_CMD_WSF_EBCDIC:
		this->WriteStructuredField(mpData, 1);
		break;

	/* No Operation */
	case IBM3270DS_CMD_NOP:
		break;
	}
}

/***********************************************************
 Handles a read request for the given screen partition.

 @param nPartId id of the partition to read
 @param nOpCode the read operation code
***********************************************************/
void CClient3270::ReadPartition(int nPartId, int nOpCode)
{
	switch (nOpCode) {
	
	/* Query */
	case IBM3270DS_SF_RP_QUERY:
		/* The partition must be FF */
		if (nPartId != 0xFF)
			break;

		/* We don't support anything :( */
		array<Byte>^ mpReply = {
			IBM3270DS_AID_SF, 
			 0x00, /* Length byte */
			 0x04, /* Length byte */
			 IBM3270DS_SF_QUERY_REPLY, 
			 IBM3270DS_SF_QR_NULL
		};
		m_mpClient->Send(mpReply);
		break;
	}
}

/***********************************************************
 The callback function for receiving data.

 @param mpData the data received
***********************************************************/
void CClient3270::ReceiveCallback(array<Byte>^ mpData)
{
	this->ParseStream(mpData, mpData->Length);
}

/***********************************************************
 Repeats a character until given address.

 @param mpData the data received
***********************************************************/
void CClient3270::RepeatToAddress(Byte chChar, int nStopAddr)
{
	bool fWrap = (nStopAddr <= m_nBufferPos);

	for (int i = m_nBufferPos; i < m_mpCharBuffer->Length; i++) {
		/* Write to the character buffer */
		C3270Char^ mpChar = gcnew C3270Char(chChar);
		m_mpCharBuffer[i] = mpChar;
		m_nBufferPos = i + 1;

		/* Wrap to the top of the buffer */
		if (i + 1 >= m_mpCharBuffer->Length && fWrap) {
			m_nBufferPos = 0;
			i = 0;
		}

		/* Break out if we're at the stop address */
		if ((nStopAddr > 0 && i == nStopAddr - 1) || (nStopAddr == 0 && i == 0))
			break;
	}
}

/***********************************************************
 Resets the modified flag on all characters in the buffer.
***********************************************************/
void CClient3270::ResetAllFieldMDTs()
{
	for (int i = 0; i < m_mpCharBuffer->Length; i++)
		m_mpCharBuffer[i]->Modified = false;
}

/***********************************************************
 Draws the character buffer on the canvas.
***********************************************************/
void CClient3270::RepaintScreen()
{
	this->RepaintScreen(0, m_mpCharBuffer->Length - 1, true);
}

/***********************************************************
 Draws the character buffer on the canvas.

 @param nStartPos the starting position of the pen
 @param nEndPos the last position to redraw
 @param fErase flag to erase the canvas
***********************************************************/
void CClient3270::RepaintScreen(int nStartPos, int nEndPos, bool fErase)
{
	Brush^ mpBrush;
	int nFieldStart;
	bool fMasked;

	/* Bounds check */
	if (nStartPos < 0 || nEndPos >= m_mpCharBuffer->Length)
		return;

	/* Notify the client that we're repainting the screen */
	if (m_mpOnPaintCbk != nullptr)
		m_mpOnPaintCbk->Invoke(nStartPos, nEndPos, fErase);

	if (m_pnlCanvas == nullptr)
		return;

	Graphics^ mpCanvas = m_pnlCanvas->CreateGraphics();
	if (fErase)
		mpCanvas->Clear(m_pnlCanvas->BackColor);

	/* Calculate block size */
	float nColWidth = 
		(float)m_pnlCanvas->Bounds.Width / (this->Size.Width + 5);
	float nRowHeight = 
		(float)m_pnlCanvas->Bounds.Height / (this->Size.Height + 1);

	/* Calculate the font size */
	Font^ mpFont = gcnew Font(FontFamily::GenericMonospace, 24);
	SizeF^ mpFontSize = mpCanvas->MeasureString("W", mpFont);
	float nNewFontSize = (24 * (nColWidth / mpFontSize->Width)) + 2;
	mpFont = gcnew Font(mpFont->FontFamily, nNewFontSize);
	
	/* Draw each character on the canvas */
	for (int i = nStartPos; i <= nEndPos; i++) {
		/* Get the current block */
		int nRow = (int)Math::Floor(i / this->Size.Width);
		int nCol = i - (nRow * this->Size.Width);
		RectangleF^ mpBlock = gcnew RectangleF( 
			(float) nCol * nColWidth, 
			(float) nRow * nRowHeight, 
			(float) nColWidth, 
			(float) nRowHeight
		);
		
		/* Draw the cursor */
		if (i == m_nCursorPos) 
			mpCanvas->FillRectangle(Brushes::Gray, *mpBlock);
		else
			mpCanvas->FillRectangle(Brushes::Black, *mpBlock);

		/* Get the brush */
		if (m_mpCharBuffer[i]->StartField) {
			mpBrush = m_mpCharBuffer[i]->GetPaintBrush(m_fBaseColor);
			fMasked = false;
		} else {
			nFieldStart = this->FindStartOfField(i);
			mpBrush = (nFieldStart == -1 ? Brushes::Green :
				m_mpCharBuffer[nFieldStart]->GetPaintBrush(m_fBaseColor));
			fMasked = (nFieldStart == -1 ? false : 
				!m_mpCharBuffer[nFieldStart]->Protected && 
				m_mpCharBuffer[nFieldStart]->Display == C3270Char::DisplayOptions::Dark);
		}
		
		/* And finally, draw the character */
		mpCanvas->DrawString( 
			m_mpCharBuffer[i]->ToString(fMasked), 
			mpFont, 
			mpBrush, 
			*mpBlock
		);
	}
}

/***********************************************************
 The callback function for sending data.
***********************************************************/
void CClient3270::SendCallback()
{
	
}

/***********************************************************
 Unserializes the flags in the write control character.

 @param mpData the data buffer to examine
 @param nOffset the offset position to start reading

 @return TRUE for WCC found, FALSE otherwise
***********************************************************/
bool CClient3270::UnserializeWCC(array<Byte>^ mpData, int nOffset)
{
	/* Check for WCC */
	if (nOffset + 1 >= mpData->Length)
		return false;

	Byte chWCC = mpData[nOffset];

	/* Bit 7: Reset MDT */
	if (chWCC & 0x01)
		this->ResetAllFieldMDTs();
	
	/* Bit 6: Unlock Keyboard */
	if (chWCC >> 1 & 0x01)
		m_fKeybLocked = false;

	/* Bit 5: Sound Alarm */
	m_fSoundAlaram = (chWCC >> 2 & 0x01);

	/* Bit 4: Start Printer */
	m_fStartPrinter = (chWCC >> 3 & 0x01);

	/* Bits 2-3: Print Line Length */
	if ((chWCC >> 4 & 0x03) == 0x00)
		m_nLineLength = 132;
	else if ((chWCC >> 4 & 0x03) == 0x01)
		m_nLineLength = 40;
	else if ((chWCC >> 4 & 0x03) == 0x02)
		m_nLineLength = 64;
	else if ((chWCC >> 4 & 0x03) == 0x03)
		m_nLineLength = 80;

	/* Bit 1: WCC Reset (Ignored) */

	/* Bit 0: Unknown (Ignored) */

	return true;
}

/***********************************************************
 Writes data to the character buffer.

 @param mpData the data buffer to examine
 @param nOffset the offset position to start reading
 @param nLength the length of the data to write
***********************************************************/
void CClient3270::WriteBuffer(array<Byte>^ mpData, int nOffset, int nLength)
{
	C3270Char^ mpChar;

	for (int i = nOffset; i < mpData->Length; i++) {
		/* Make sure we don't exceed the data length */
		if (i >= nLength)
			break;

		/* Get the order and adjust the position accordingly */
		i = this->InterpretOrder(mpData, i);
		if (i >= mpData->Length)
			break;

		/* Write to the character buffer */
		mpChar = gcnew C3270Char(mpData[i]);
		m_mpCharBuffer[m_nBufferPos] = mpChar;
		m_nBufferPos++;
		
		/* Wrap so we don't go off the screen */
		if (m_nBufferPos >= m_mpCharBuffer->Length) 
			m_nBufferPos = 0;
	}

	this->RepaintScreen();
}

/***********************************************************
 Writes an output structured field.

 @param mpData the data buffer to examine
 @param nOffset the offset position to start reading
***********************************************************/
void CClient3270::WriteStructuredField(array<Byte>^ mpData, int nOffset)
{
	int nPId;
	int nType;

	for (int i = nOffset; i < mpData->Length; i++) {
		/* Get the field length */
		if (i + 1 >= mpData->Length)
			break;
		int nLength = (mpData[i] << 8) + mpData[i + 1];

		/* Check for an invalid field length */
		if (nLength > 0 && nLength < 3)
			break;

		/* Get the structured field id (one byte first) */
		int nSFId = mpData[i + 2];

		switch (nSFId) {
		
		/* Read Partition */
		case IBM3270DS_SF_READ_PART:
			/* Bounds check */
			if (nLength < 5 || (i + 5) > mpData->Length)
				break;

			/* Get the partition id and type id */
			nPId = mpData[i + 3];
			nType = mpData[i + 4];

			this->ReadPartition(nPId, nType);
			break;

		default:
			/* Unsupported SFID */
			i += nLength;
			continue;
		}
	}
}
