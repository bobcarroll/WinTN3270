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
#include "CWndMain.h"

using namespace WinTN3270;
using namespace System;
using namespace System::Net::Security;
using namespace System::Security::Cryptography::X509Certificates;
using namespace System::Windows;

/***********************************************************
 Handles the form load event.
***********************************************************/
Void CWndMain::CWndMain_Load(Object^ sender, EventArgs^ e)
{
	/* Prep the canvas */
	pbxCanvas->BackColor = Color::Black;
	
	m_mpClient = gcnew CClient3270("mvs.example.com", 992, 2);
	m_mpClient->Canvas = pbxCanvas;
	m_mpClient->CertificateErrorCallback = gcnew CClient3270::OnCertPolicyError(
		this,
		&CWndMain::m_mpClient_CertPolicyError
	);
	m_mpClient->Connect(true);
}

/***********************************************************
 Handles the key down event for the canvas.
***********************************************************/
System::Void CWndMain::CWndMain_KeyDown(System::Object^ sender, KeyEventArgs^ e)
{
	switch (e->KeyCode) {

	case ConsoleKey::LeftArrow:
		m_mpClient->MoveCursor(-1, 0, true);
		break;

	case ConsoleKey::RightArrow:
		m_mpClient->MoveCursor(1, 0, true);
		break;

	case ConsoleKey::UpArrow:
		m_mpClient->MoveCursor(0, -1, true);
		break;

	case ConsoleKey::DownArrow:
		m_mpClient->MoveCursor(0, 1, true);
		break;

	case ConsoleKey::F12:
		m_mpClient->KeyPress(gcnew KeyPressEventArgs((wchar_t) ConsoleKey::F12));
		break;
	}
}

/***********************************************************
 Handles the key press event for the canvas.
***********************************************************/
System::Void CWndMain::CWndMain_KeyPress(System::Object^ sender, KeyPressEventArgs^ e)
{
	m_mpClient->KeyPress(e, this->ModifierKeys);
}

/***********************************************************
 Handles the form paint event.
***********************************************************/
void CWndMain::OnPaint(PaintEventArgs^ e)
{
	/* Re-draw the terminal display */
	m_mpClient->RepaintScreen();
}

/***********************************************************
 Callback for handling SSL policy errors.

 @param mpCertificate the cert to validate
 @param mpChain the certificate chain
 @param ePolicyErrors any policy errors

 @return TRUE for valid, FALSE otherwise
***********************************************************/
bool CWndMain::m_mpClient_CertPolicyError(X509Certificate^ mpCertificate, 
	X509Chain^ mpChain, SslPolicyErrors ePolicyErrors)
{
	String^ mpErrors = "";

	if ((ePolicyErrors & SslPolicyErrors::RemoteCertificateChainErrors) == SslPolicyErrors::RemoteCertificateChainErrors)
		mpErrors += "\n\n* The security certificate is not from a trusted authority.";
	if ((ePolicyErrors & SslPolicyErrors::RemoteCertificateNameMismatch) == SslPolicyErrors::RemoteCertificateNameMismatch)
		mpErrors += "\n\n* The name on the security certificate is invalid or	does not match the name of the remote host.";
	if ((ePolicyErrors & SslPolicyErrors::RemoteCertificateNotAvailable) == SslPolicyErrors::RemoteCertificateNotAvailable)
		mpErrors += "\n\n* The security certificate is not available.";

	return MessageBox::Show("There is a problem with remote host's security certificate!" 
		+ mpErrors + "\n\nDo you want to proceed?", "Security Warning", 
		MessageBoxButtons::YesNo, MessageBoxIcon::Exclamation, 
		MessageBoxDefaultButton::Button2) == Forms::DialogResult::Yes;
}
