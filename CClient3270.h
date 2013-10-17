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

#pragma once

#include "CTelnetClient.h"
#include "C3270Char.h"

namespace WinTN3270
{
	/***********************************************************
	 Telnet client for IBM 3270.
	***********************************************************/
	public ref class CClient3270
	{

	public: /* Public Delegates */
		typedef CTelnetClient::OnCertPolicyError OnCertPolicyError;

	private: /* Buffer Address Conversion Table */
		static cli::array<int>^ m_mpAddrTbl = 
			{ 0x40, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
			  0xC8, 0xC9, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
			  0x50, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
			  0xD8, 0xD9, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
			  0x60, 0x61, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
			  0xE8, 0xE9, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
			  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
			  0xF8, 0xF9, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F };

	public: // Public Delegates
		delegate void OnPaintEvent(int nStartPos, int nEndPos, bool fErase);

	private: // Private Member Attributes
		bool m_fBaseColor;
		bool m_fKeybLocked;
		bool m_fSoundAlaram;
		bool m_fStartPrinter;
		CTelnetClient^ m_mpClient;
		cli::array<C3270Char^>^ m_mpCharBuffer;
		OnPaintEvent^ m_mpOnPaintCbk;
		int m_nActionId;
		int m_nBufferPos;
		int m_nCursorPos;
		int m_nCols;
		int m_nLastField;
		int m_nLineLength;
		int m_nModel;
		int m_nRows;
		System::Windows::Forms::PictureBox^ m_pnlCanvas;

	public: // Public Properties
		property bool BaseColorMode
		{
			bool get()
			{
				return m_fBaseColor;
			}

			void set(bool fValue)
			{
				m_fBaseColor = fValue;
			}
		}

		property cli::array<C3270Char^>^ Buffer
		{
			cli::array<C3270Char^>^ get() { return m_mpCharBuffer; }
		}

		property System::Windows::Forms::PictureBox^ Canvas
		{
			System::Windows::Forms::PictureBox^ get() 
			{
				return m_pnlCanvas;
			}

			void set(System::Windows::Forms::PictureBox^ pnlCanvas)
			{
				m_pnlCanvas = pnlCanvas;
			}
		}

		property OnCertPolicyError^ CertificateErrorCallback
		{
			OnCertPolicyError^ get()
			{
				return m_mpClient->CertificateErrorCallback;
			}

			void set(OnCertPolicyError^ mpValue)
			{
				m_mpClient->CertificateErrorCallback = mpValue;
			}
		}

		property bool Connected
		{
			bool get() { return m_mpClient->Connected; }
		}

		property bool Locked
		{
			bool get() { return m_fKeybLocked; }
		}

		property int Model
		{
			int get() { return m_nModel; }
		}

		property OnPaintEvent^ PaintCallback
		{
			OnPaintEvent^ get() 
			{ 
				return m_mpOnPaintCbk;
			}

			void set(OnPaintEvent^ mpValue)
			{
				m_mpOnPaintCbk = mpValue;
			}
		}

		property System::Drawing::Size Size
		{
			System::Drawing::Size get()
			{
				System::Drawing::Size oSize;
				oSize.Height = m_nRows;
				oSize.Width = m_nCols;

				return oSize;
			}
		}

	private: /* Private Member Functions */
		void __Constructor(System::String^ cchIPAddr, int nPort, int nModel);
		int ConvertBufferAddress(System::Byte chFirst, System::Byte chSecond);
		cli::array<System::Byte>^ CreateActionArray(System::Byte chActionId);
		bool DoInputAction(wchar_t chKeyChar);
		void EraseAllUnprotected();
		void EraseScreen();
		int FindStartOfField(int nOffset);
		int InterpretOrder(cli::array<System::Byte>^ mpData, int nOffset);
		bool IsProtectedField(int nOffset);
		void JumpToNextUnprotectedField();
		void JumpToPrevUnprotectedField();
		cli::array<System::Byte>^ MergeModifiedFields(cli::array<System::Byte>^ mpData);
		void MoveCursor(int nStep);
		void ParseStream(cli::array<System::Byte>^ mpData, int nLength);
		void ReadPartition(int nPartId, int nOpCode);
		void ReceiveCallback(cli::array<System::Byte>^ mpData);
		void RepaintScreen(int nStartPos, int nEndPos, bool fErase);
		void RepeatToAddress(System::Byte chChar, int nStopAddr);
		void ResetAllFieldMDTs();
		void SendCallback();
		bool UnserializeWCC(cli::array<System::Byte>^ mpData, int nOffset);
		void WriteBuffer(cli::array<System::Byte>^ mpData, int nOffset, int nLength);
		void WriteStructuredField(cli::array<System::Byte>^ mpData, int nOffset);

	public: /* Public Member Functions */
		CClient3270(System::String^ cchIPAddr, int nPort);
		CClient3270(System::String^ cchIPAddr, int nPort, int nModel);
		~CClient3270();
		void Connect();
		void Connect(bool fSecure);
		void Disconnect();
		void KeyPress(System::Windows::Forms::KeyPressEventArgs^ mpArgs);
		void KeyPress(System::Windows::Forms::KeyPressEventArgs^ mpArgs, 
			System::Windows::Forms::Keys nModifiers);
		void MoveCursor(int x, int y);
		void MoveCursor(int x, int y, bool fRelative);
		void RepaintScreen();
	};
}
