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

namespace WinTN3270
{
	/***********************************************************
	 Character object for 3270 terminals.
	***********************************************************/
	public ref class C3270Char
	{

	public: /* Public Enums */
		ref class DisplayOptions
		{
		public:
			static const int NormalNoLPDetect = 0;
			static const int NormalLPDetect = 1;
			static const int Bright = 2;
			static const int Dark = 3;
		};

	private: /* Private Member Attributes */
		wchar_t m_chInitChar;
		wchar_t m_chChar;
		bool m_fAutoSkip;
		bool m_fModified;
		bool m_fNumeric;
		bool m_fProtected;
		bool m_fStartField;
		int m_nDisplay;

	public: /* Public Properties */
		property bool AutoSkip
		{
			bool get()
			{
				return m_fAutoSkip;
			}
		}

		property wchar_t Character
		{
			wchar_t get()
			{
				return m_chChar;
			}

			void set(wchar_t chValue)
			{
				m_chChar = chValue;
			}
		}

		property int Display
		{
			int get()
			{
				return m_nDisplay;
			}
		}

		property bool Modified
		{
			bool get()
			{
				return m_fModified;
			}

			void set(bool fValue)
			{
				/* This is only for the field-start */
				if (!m_fStartField)
					return;

				/* Flip bit 7 */
				m_chChar = (fValue ? m_chChar | 0x01 : m_chChar & ~0x01);
				m_fModified = fValue;
			}
		}

		property int NumericOnly
		{
			int get()
			{
				return m_fNumeric;
			}
		}

		property bool Protected
		{
			bool get()
			{
				return m_fProtected;
			}
		}

		property bool StartField
		{
			bool get()
			{
				return m_fStartField;
			}
		}

	private: /* Private Member Functions */
		void UnserializeAttributes();

	public: /* Public Member Functions */
		C3270Char();
		C3270Char(System::Byte chChar);
		C3270Char(System::Byte chChar, bool fStartField);
		void __Constructor(System::Byte chChar, bool fStartField);
		System::Drawing::Brush^ GetPaintBrush();
		System::Drawing::Brush^ GetPaintBrush(bool fBaseColor);
		virtual System::String^ ToString() override;
		System::String^ ToString(bool fMasked);
	};
}
