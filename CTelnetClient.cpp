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
#include "CTelnetClient.h"

using namespace WinTN3270;
using namespace System;
using namespace System::Net;
using namespace System::Net::Security;
using namespace System::Net::Sockets;
using namespace System::Security::Authentication;
using namespace System::Security::Cryptography::X509Certificates;
using namespace System::Threading;
using namespace System::Collections;

/* The length of this delegate is absurd */
typedef RemoteCertificateValidationCallback RemoteCertValidateCbk;

/***********************************************************
 Creates a new TELNET client.

 @param cchIPAddr the IP address of the remote host
 @param nPort the remote TCP port
***********************************************************/
CTelnetClient::CTelnetClient(String^ cchIPAddr, int nPort)
{
	/* Setup the stream socket */
	m_mpSocket = gcnew Socket( 
		AddressFamily::InterNetwork, 
		SocketType::Stream, 
		ProtocolType::Tcp
	);
	IPHostEntry^ mpRemoteHost = Dns::GetHostEntry(cchIPAddr);
	if (mpRemoteHost->AddressList->Length == 0)
		throw gcnew Exception("No IP addresses are associated with the host record.");
	m_mpRemoteIP = gcnew IPEndPoint(mpRemoteHost->AddressList[0], nPort);

	/* Initialize stuff */
	m_mpRcvCommandBuffer = gcnew array<Byte>(0);
	m_mpRcvDataBuffer = gcnew array<Byte>(0);
	m_mpTransmQueue = gcnew array<Byte>(0);
	m_mpServerOpts = gcnew Hashtable();
	m_mpClientOpts = gcnew Hashtable();
	m_cchRemoteHost = cchIPAddr;
	m_fGoAhead = true;

	m_mpSendEvent = gcnew EventWaitHandle(true, EventResetMode::ManualReset);

	/* Set default options */
	this->FullDuplex = true;
}

/***********************************************************
 Disconnects from the remote host.
***********************************************************/
CTelnetClient::~CTelnetClient()
{
	this->Disconnect();
}

/***********************************************************
 Callback for validating SSL certificates.

 @param mpSender the calling object
 @param mpCertificate the cert to validate
 @param mpChain the certificate chain
 @param ePolicyErrors any policy errors

 @return TRUE for valid, FALSE otherwise
***********************************************************/
bool CTelnetClient::CertValidationCallback(Object^ mpSender, 
	X509Certificate^ mpCertificate, X509Chain^ mpChain, 
	SslPolicyErrors ePolicyErrors)
{
	/* Accept all certs without policy errors */
	if (ePolicyErrors == SslPolicyErrors::None)
		return true;

	/* Get the callback function */
	Monitor::Enter(this);
	OnCertPolicyError^ mpCertErrorCbk = m_mpCertErrorCbk;
	Monitor::Exit(this);

	/* If we have no policy error handler, then don't accept */
	if (mpCertErrorCbk == nullptr)
		return false;

	/* Otherwise let the handler decide */
	return mpCertErrorCbk->Invoke( 
		mpCertificate, 
		mpChain, 
		ePolicyErrors
	);
}

/***********************************************************
 Opens a new connection to the remote host.
***********************************************************/
void CTelnetClient::Connect()
{
	this->Connect(false);
}

/***********************************************************
 Opens a new connection to the remote host.

 @param fSecure enable SSL
***********************************************************/
void CTelnetClient::Connect(bool fSecure)
{
	/* Make sure we're not already connected */
	if (m_mpSocket->Connected)
		return;

	/* Connect to the remote host */
	if (m_mpRemoteIP == nullptr) 
		throw gcnew Exception("Remote end-point is not set.");
	m_mpSocket->Connect(m_mpRemoteIP);
	m_mpSocketStream = gcnew NetworkStream(m_mpSocket);

	/* If SSL is enabled, then wrap the stream */
	if (fSecure) {
		m_mpSocketStream = gcnew SslStream( 
			m_mpSocketStream, 
			false, 
			gcnew RemoteCertValidateCbk(this, &CTelnetClient::CertValidationCallback)
		);
		
		try {
			((SslStream^) m_mpSocketStream)->AuthenticateAsClient(m_cchRemoteHost);
		} catch(AuthenticationException^ e) {
			m_mpSocket->Disconnect(true);
			delete e;

			return;
		}
	}

	/* Create the new state struct */
	SOCKETRCVSTATE^ mpRcvState = gcnew SOCKETRCVSTATE;
	mpRcvState->mpBuffer = gcnew array<Byte>(256);
	mpRcvState->mpAscCallback = m_mpReceiveAscCbk;
	mpRcvState->mpBinCallback = m_mpReceiveBinCbk;
	mpRcvState->mpClient = this;
	mpRcvState->mpStream = m_mpSocketStream;

	/* Start receving data */
	m_mpSocketStream->BeginRead(
		mpRcvState->mpBuffer, 
		0, 
		mpRcvState->mpBuffer->Length, 
		gcnew AsyncCallback(&CTelnetClient::ReceiveProc), 
		mpRcvState
	);
}

/***********************************************************
 Terminates the active stream connection.
***********************************************************/
void CTelnetClient::Disconnect()
{
	if (!m_mpSocket->Connected)
		return;

	m_mpSocket->Disconnect(true);
}

/***********************************************************
 Send all pending transmit data to the remote host.
***********************************************************/
void CTelnetClient::FlushTransmitQueue()
{
	/* Copy to our buffer and clear the queue */
	Monitor::Enter(this);
	array<Byte>^ mpData = gcnew array<Byte>(m_mpTransmQueue->Length);
	array<Byte>::Copy(m_mpTransmQueue, mpData, mpData->Length);
	m_mpTransmQueue = gcnew array<Byte>(0);
	Monitor::Exit(this);

	/* We should always have something to send, but... */
	if (mpData->Length == 0)
		return;

	/* Reset the 'go ahead' flag if we're on half-duplex */
	if (!this->FullDuplex)
		m_fGoAhead = false;

	SOCKETSNDSTATE^ mpSndState = gcnew SOCKETSNDSTATE;
	mpSndState->mpCallback = m_mpSendCbk;
	mpSndState->mpClient = this;
	mpSndState->mpStream = m_mpSocketStream;

	/* SslStream does not allow concurrent asynchronous writes, so we have to wait
	   for the last write to finish. */
	m_mpSendEvent->WaitOne();
	m_mpSendEvent->Reset();

	try {
		m_mpSocketStream->BeginWrite( 
			mpData, 
			0, 
			mpData->Length, 
			gcnew AsyncCallback(&CTelnetClient::SendProc), 
			mpSndState
		);
	} catch (SocketException^ e) {
		throw e;
	}

	if (mpSndState->mpCallback != nullptr)
		mpSndState->mpCallback->Invoke();
}

/***********************************************************
 Gets existing option values for the client.

 @param nId the option id
 
 @return a byte array containg the value
***********************************************************/
array<Byte>^ CTelnetClient::GetClientOption(int nId)
{
	/* Detect if the option isn't set */
	if (!m_mpClientOpts->ContainsKey(nId))
		return nullptr;

	try {
		/* First try a direct cast */
		return (array<Byte>^)m_mpClientOpts[nId];
	} catch (InvalidCastException^ e) {
		delete e;
	}

	try {
		/* Maybe it's a boolean? */
		return (array<Byte>^)BitConverter::GetBytes( (bool) m_mpClientOpts[nId]);
	} catch (InvalidCastException^ e) {
		delete e;
	}

	/* This should never happen */
	return gcnew array<Byte>(0);
}

/***********************************************************
 Gets existing option values for the server.

 @param nId the option id
 
 @return a byte array containg the value
***********************************************************/
array<Byte>^ CTelnetClient::GetServerOption(int nId)
{
	/* Detect if the option isn't set */
	if (!m_mpServerOpts->ContainsKey(nId))
		return nullptr;

	try {
		/* First try a direct cast */
		return (array<Byte>^)m_mpServerOpts[nId];
	} catch (InvalidCastException^ e) {
		delete e;
	}

	try {
		/* Maybe it's a boolean? */
		return (array<Byte>^)BitConverter::GetBytes((bool)m_mpServerOpts[nId]);
	} catch (InvalidCastException^ e) {
		delete e;
	}

	/* This should never happen */
	return gcnew array<Byte>(0);
}

/***********************************************************
 Interprets and handles TELNET commands.

 @param mpData the receive data buffer

 @return the sanitized array
***********************************************************/
array<Byte>^ CTelnetClient::InterpretCommands(array<Byte>^ mpData)
{
	bool fNoDataFlush = (m_mpRcvDataBuffer->Length != 0);
	bool fNoCommandFlush = false;
	array<Byte>^ mpNewDataBuffer;
	array<Byte>^ mpResp;
	bool fResult;

	/* Merge the incoming data with the buffer */
	int nStartIndex = m_mpRcvCommandBuffer->Length;
	if (mpData != nullptr) {
		array<Byte>::Resize(
			m_mpRcvCommandBuffer,
			m_mpRcvCommandBuffer->Length + mpData->Length
		);
		array<Byte>::Copy(mpData, 0, m_mpRcvCommandBuffer, nStartIndex, mpData->Length);
	}

	if (m_mpRcvCommandBuffer->Length == 0)
		return nullptr;

	for (int i = 0; i < m_mpRcvCommandBuffer->Length; i++) {
		nStartIndex = i;

		/* Check for the end-of-record flag */
		fNoDataFlush = this->TestServerOption(TELNET_OPTION_END_OF_RECORD);

		/* There should never be a single FF character at the end
		   of the buffer. So if we see one, wait for more data. */
		if (m_mpRcvCommandBuffer[i] == TELNET_COMMAND_IAC && 
				i == m_mpRcvCommandBuffer->Length - 1)
		{
			fNoCommandFlush = true;
			break;
		}

		/* If we're in sub-option negotiation and we don't
		   see an IAC, then capture the option value */
		if ((m_mpSubOpt != nullptr && m_mpRcvCommandBuffer[i] != TELNET_COMMAND_IAC) ||
			(m_mpSubOpt != nullptr && m_mpRcvCommandBuffer[i] == TELNET_COMMAND_IAC
				&& m_mpRcvCommandBuffer[i + 1] == TELNET_COMMAND_IAC))
		{
			array<Byte>::Resize(
				m_mpSubOpt->pchValue, 
				m_mpSubOpt->pchValue->Length + 1
			);
			m_mpSubOpt->pchValue[m_mpSubOpt->pchValue->Length - 1] = m_mpRcvCommandBuffer[i];

			if (m_mpRcvCommandBuffer[i] == TELNET_COMMAND_IAC)
				i++;

			continue;
		}

		/* Two IAC bytes in a row indicate one FF character,
		   so it's only a command if we see one. */
		if (m_mpRcvCommandBuffer[i] != TELNET_COMMAND_IAC ||
			(m_mpRcvCommandBuffer[i] == TELNET_COMMAND_IAC &&
			m_mpRcvCommandBuffer[i + 1] == TELNET_COMMAND_IAC))
		{
			array<Byte>::Resize( 
				m_mpRcvDataBuffer,
				m_mpRcvDataBuffer->Length + 1
			);
			m_mpRcvDataBuffer[m_mpRcvDataBuffer->Length - 1] = m_mpRcvCommandBuffer[i];

			if (m_mpRcvCommandBuffer[i] == TELNET_COMMAND_IAC)
				i++;

			continue;
		}

		/* If we're at the end of the loop, then buffer the command */
		if (i == m_mpRcvCommandBuffer->Length - 1) {
			fNoCommandFlush = true;	
			break;
		}

		/* Process the given command per RFC0854 */
		switch (m_mpRcvCommandBuffer[i + 1]) {

		/* End of Record */
		case (TELNET_COMMAND_END_OF_RECORD):
			/* Flush the data buffer */
			mpNewDataBuffer = gcnew array<Byte>(m_mpRcvDataBuffer->Length);
			array<Byte>::Copy( 
				m_mpRcvDataBuffer, 
				mpNewDataBuffer, 
				m_mpRcvDataBuffer->Length
			);
			array<Byte>::Resize(m_mpRcvDataBuffer, 0);

			/* Exit and wait for more data */
			fNoCommandFlush = true;
			nStartIndex = (i + 2);
			break;

		/* End sub-option negotiation */
		case (TELNET_COMMAND_SE):
			/* Get the option value */
			if (m_mpSubOpt->fValueRequired)
				m_mpSubOpt->pchValue = this->GetClientOption(m_mpSubOpt->nOptionId);

			/* Send the reply */
			mpResp = gcnew array<Byte>(m_mpSubOpt->pchValue->Length + 6);
			mpResp[0] = TELNET_COMMAND_IAC;
			mpResp[1] = TELNET_COMMAND_SB;
			mpResp[2] = m_mpSubOpt->nOptionId;
			mpResp[3] = 0x00;
			array<Byte>::Copy(
				m_mpSubOpt->pchValue,
				0,
				mpResp, 
				4,
				m_mpSubOpt->pchValue->Length
			);
			mpResp[m_mpSubOpt->pchValue->Length + 4 + 0] = TELNET_COMMAND_IAC;
			mpResp[m_mpSubOpt->pchValue->Length + 4 + 1] = TELNET_COMMAND_SE;
			this->Transmit(mpResp);

			m_mpSubOpt = nullptr;
			i++;
			break;

		/* No operation */
		case (TELNET_COMMAND_NOP):
			i += 2;
			break;
 
		/* Are you there? */
		case (TELNET_COMMAND_ARE_YOU_THERE):
			mpResp = gcnew array<Byte>(2);
			mpResp[0] = TELNET_COMMAND_IAC;
			mpResp[1] = TELNET_COMMAND_NOP;
			this->Transmit(mpResp);
			i += 2;
			break;

		/* Go Ahead */
		case (TELNET_COMMAND_GA):
			m_fGoAhead = true;
			this->FlushTransmitQueue();
			i += 2;
			break;

		/* Begin sub-option negotiation */
		case (TELNET_COMMAND_SB):
			/* Check for all parameters */
			if (i + 3 >= m_mpRcvCommandBuffer->Length) {
				fNoCommandFlush = true;	
				break;
			}

			/* We're in a sub-option now */
			m_mpSubOpt = gcnew TNSUBOPTION;
			m_mpSubOpt->nOptionId = m_mpRcvCommandBuffer[i + 2];
			m_mpSubOpt->pchValue = gcnew array<Byte>(0);
			m_mpSubOpt->fValueRequired = BitConverter::ToBoolean( m_mpRcvCommandBuffer, i + 3);
			i += 3;
			break;


		/* Client is offering an option */
		case (TELNET_COMMAND_WILL):
			/* Check for all parameters */
			if (i + 2 >= m_mpRcvCommandBuffer->Length) {
				fNoCommandFlush = true;	
				break;
			}

			/* Get the client option */
			fResult = (this->GetClientOption(m_mpRcvCommandBuffer[i + 2]) != nullptr);

			/* Send the reply */
			mpResp = gcnew array<Byte>(3);
			mpResp[0] = TELNET_COMMAND_IAC;
			mpResp[1] = (fResult ? TELNET_COMMAND_DO : TELNET_COMMAND_DONT);
			mpResp[2] = m_mpRcvCommandBuffer[i + 2];
			this->Transmit(mpResp);
			i += 2;
			break;

		/* Client won't do an option */
		case (TELNET_COMMAND_WONT):
			/* Check for all parameters */
			if (i + 2 >= m_mpRcvCommandBuffer->Length) {
				fNoCommandFlush = true;	
				break;
			}

			/* Attempt to unset the option */
			fResult = this->SetServerOption(m_mpRcvCommandBuffer[i + 2], nullptr);

			// Send the reply
			mpResp = gcnew array<Byte>(3);
			mpResp[0] = TELNET_COMMAND_IAC;
			mpResp[1] = (!fResult ? TELNET_COMMAND_DO : TELNET_COMMAND_DONT);
			mpResp[2] = m_mpRcvCommandBuffer[i + 2];
			this->Transmit(mpResp);
			i += 2;
			break;

		/* Client wants an option */
		case (TELNET_COMMAND_DO):
			/* Check for all parameters */
			if (i + 2 >= m_mpRcvCommandBuffer->Length) {
				fNoCommandFlush = true;	
				break;
			}

			/* Attempt to set the option */
			fResult = this->SetServerOption(m_mpRcvCommandBuffer[i + 2], true);

			// Send the reply
			mpResp = gcnew array<Byte>(3);
			mpResp[0] = TELNET_COMMAND_IAC;
			mpResp[1] = (fResult ? TELNET_COMMAND_WILL : TELNET_COMMAND_WONT);
			mpResp[2] = m_mpRcvCommandBuffer[i + 2];
			this->Transmit(mpResp);
			i += 2;
			break;

		/* Client doesn't want an option */
		case (TELNET_COMMAND_DONT):
			/* Check for all parameters */
			if (i + 2 >= m_mpRcvCommandBuffer->Length) {
				fNoCommandFlush = true;	
				break;
			}

			/* Attempt to unset the option */
			fResult = this->SetServerOption(m_mpRcvCommandBuffer[i + 2], nullptr);

			// Send the reply
			mpResp = gcnew array<Byte>(3);
			mpResp[0] = TELNET_COMMAND_IAC;
			mpResp[1] = (!fResult ? TELNET_COMMAND_WILL : TELNET_COMMAND_WONT);
			mpResp[2] = m_mpRcvCommandBuffer[i + 2];
			this->Transmit(mpResp);
			i += 2;
			break;

		/* Unsupported command */
		default:
			i++;
		}

		if (fNoCommandFlush)
			break;
	}

	if (fNoCommandFlush) {
		/* Shift the remaining data to the start of the buffer
		   because we're waiting for the complete request to arrive */
		array<Byte>::Copy(
			m_mpRcvCommandBuffer,
			nStartIndex, 
			m_mpRcvCommandBuffer,
			0,
			m_mpRcvCommandBuffer->Length - nStartIndex
		);
		array<Byte>::Resize(
			m_mpRcvCommandBuffer, 
			m_mpRcvCommandBuffer->Length - nStartIndex
		);
	} else {
		/* We're at the end, empty the buffer */
		array<Byte>::Resize(m_mpRcvCommandBuffer, 0);
	}

	if (!fNoDataFlush && m_mpRcvDataBuffer->Length > 0) {
		/* Move data from the receive buffer */
		mpNewDataBuffer = gcnew array<Byte>(m_mpRcvDataBuffer->Length);
		array<Byte>::Copy( 
			m_mpRcvDataBuffer, 
			mpNewDataBuffer, 
			m_mpRcvDataBuffer->Length
		);
		array<Byte>::Resize(m_mpRcvDataBuffer, 0);
	}

	return mpNewDataBuffer;
}

/***********************************************************
 Callback function for asynchronous receiving.
***********************************************************/
void CTelnetClient::ReceiveProc(IAsyncResult^ mpResult)
{
	/* Finish the receive operation */
	SOCKETRCVSTATE^ mpRcvState = safe_cast<SOCKETRCVSTATE^>(mpResult->AsyncState);
	int nBytesRead = mpRcvState->mpStream->EndRead(mpResult);

	/* If we got some data back... */
	if (nBytesRead > 0) {
		/* Trim the fat off the new buffer */
		array<Byte>::Resize(mpRcvState->mpBuffer, nBytesRead);

		while (true) {
			/* Process TELNET commands */
			mpRcvState->mpBuffer = mpRcvState->mpClient->InterpretCommands(mpRcvState->mpBuffer);
			if (mpRcvState->mpBuffer == nullptr)
				break;
		
			if (mpRcvState->mpBuffer->Length > 0 && mpRcvState->mpClient->BinaryTransmission) {
				if (mpRcvState->mpBinCallback != nullptr)
					mpRcvState->mpBinCallback->Invoke(mpRcvState->mpBuffer);
			} else if (mpRcvState->mpBuffer->Length > 0) {
				/* Convert it to a string */
				array<wchar_t>^ pchData = gcnew array<wchar_t>(mpRcvState->mpBuffer->Length);
				array<wchar_t>::Copy(mpRcvState->mpBuffer, pchData, mpRcvState->mpBuffer->Length);
				String^ cchDataStr = gcnew String(pchData);

				if (mpRcvState->mpAscCallback != nullptr)
					mpRcvState->mpAscCallback->Invoke(cchDataStr);
			}

			/* Empty the buffer */
			mpRcvState->mpBuffer = gcnew array<Byte>(0);
		}
	}

	SOCKETRCVSTATE^ mpNewRcvState = gcnew SOCKETRCVSTATE;
	mpNewRcvState->mpBuffer = gcnew array<Byte>(256);
	mpNewRcvState->mpAscCallback = mpRcvState->mpAscCallback;
	mpNewRcvState->mpBinCallback = mpRcvState->mpBinCallback;
	mpNewRcvState->mpClient = mpRcvState->mpClient;
	mpNewRcvState->mpStream = mpRcvState->mpStream;

	try {
		/* Start receving data again */
		mpRcvState->mpStream->BeginRead( 
			mpNewRcvState->mpBuffer, 
			0, 
			mpNewRcvState->mpBuffer->Length, 
			gcnew AsyncCallback(&CTelnetClient::ReceiveProc), 
			mpNewRcvState
		);
	} catch (SocketException^ e) {
		delete e;
	}
}

/***********************************************************
 Sends string data to the remote host.

 @param cchData the string data to send
***********************************************************/
void CTelnetClient::Send(String^ cchData)
{
	Text::Encoding^ mpEncoding = Text::ASCIIEncoding::Default;
	array<Byte>^ mpBytes = (array<Byte>^)mpEncoding->GetBytes(cchData);
	this->Send(mpBytes);
}

/***********************************************************
 Sends byte data to the remote host.

 @param mpData the byte data to send
***********************************************************/
void CTelnetClient::Send(array<Byte>^ mpData)
{
	array<Byte>^ mpNewData = gcnew array<Byte>(0);
	int n = 0;

	for (int i = 0; i < mpData->Length; i++) {
		array<Byte>::Resize(
			mpNewData,
			mpNewData->Length + (mpData[i] == 0xFF ? 2 : 1)
		);
		mpNewData[n] = mpData[i];
		n++;

		/* Escape FF */
		if (mpData[i] == 0xFF) {
			mpNewData[n] = mpData[i];
			n++;
		}
	}

	/* Detect end-of-record option and append IAC EOR if needed */
	if (m_mpServerOpts->ContainsKey(TELNET_OPTION_END_OF_RECORD)) {
		array<Byte>::Resize(mpNewData, mpNewData->Length + 2);
		mpNewData[mpNewData->Length - 2] = TELNET_COMMAND_IAC;
		mpNewData[mpNewData->Length - 1] = TELNET_COMMAND_END_OF_RECORD;
	}

	this->Transmit(mpNewData);
}

/***********************************************************
 Callback function for asynchronous sending.
***********************************************************/
void CTelnetClient::SendProc(System::IAsyncResult^ mpResult)
{
	SOCKETSNDSTATE^ mpSndState = safe_cast<SOCKETSNDSTATE^>(mpResult->AsyncState);
	mpSndState->mpStream->EndWrite(mpResult);
	mpSndState->mpClient->m_mpSendEvent->Set();
}

/***********************************************************
 Enables a terminal option for the server.

 @param nId the option id
 @param fValue the option value (nullptr to unset)

 @return TRUE if the option is support, FALSE otherwise
***********************************************************/
bool CTelnetClient::SetServerOption(int nId, bool fValue)
{
	return this->SetServerOption(nId, BitConverter::GetBytes(fValue));
}

/***********************************************************
 Enables a terminal option for the server.

 @param nId the option id
 @param mpValue the option value (nullptr to unset)

 @return TRUE if the option is support, FALSE otherwise
***********************************************************/
bool CTelnetClient::SetServerOption(int nId, array<Byte>^ mpValue)
{
	/* Validate the input, and handle special cases */
	if (mpValue == nullptr && m_mpServerOpts->ContainsKey(nId))
		m_mpServerOpts->Remove(nId);
	else if (mpValue == nullptr && !m_mpServerOpts->ContainsKey(nId))
		return false;
	else if (mpValue != nullptr && m_mpServerOpts->ContainsKey(nId))
		return true;

	/* Select supported options */
	switch (nId) {

	/* Binary Transmission */
	case (TELNET_OPTION_BINARY):
		return m_mpClientOpts->ContainsKey(safe_cast<int^>(TELNET_OPTION_TERM_TYPE));

	/* Suppress Go Ahead */
	case (TELNET_OPTION_SUPPRESS_GA):
		return m_mpClientOpts->ContainsKey(safe_cast<int^>(TELNET_OPTION_SUPPRESS_GA));

	/* Terminal Type */
	case (TELNET_OPTION_TERM_TYPE):
		return m_mpClientOpts->ContainsKey(TELNET_OPTION_TERM_TYPE);

	/* End of Record */
	case (TELNET_OPTION_END_OF_RECORD):
		m_mpServerOpts->Add(TELNET_OPTION_END_OF_RECORD, true);
		return true;
	}

	/* Everything else is unsupported */
	return false;
}

/***********************************************************
 Tests for both the existance of, and the value of a
 server option.

 @param nId the option id
 
 @return a boolean representation of the value
***********************************************************/
bool CTelnetClient::TestServerOption(int nId)
{
	/* Detect if the option isn't set */
	if (!m_mpServerOpts->ContainsKey(nId))
		return false;

	try {
		return (bool)m_mpServerOpts[nId];
	} catch (InvalidCastException^ e) {
		delete e;
	}

	return false;
}

/***********************************************************
 Sends byte data to the remote host.

 @param mpData the byte data to send
***********************************************************/
void CTelnetClient::Transmit(array<Byte>^ mpData)
{
	/* Copy our buffer to the queue */
	Monitor::Enter(this);
	int nQueueIndex = m_mpTransmQueue->Length;
	array<Byte>::Resize( 
		m_mpTransmQueue, 
		m_mpTransmQueue->Length + mpData->Length
	);
	array<Byte>::Copy(
		mpData,
		0,
		m_mpTransmQueue, 
		nQueueIndex,
		mpData->Length
	);
	Monitor::Exit(this);

	if (this->FullDuplex || (!this->FullDuplex && m_fGoAhead))
		this->FlushTransmitQueue();
}
