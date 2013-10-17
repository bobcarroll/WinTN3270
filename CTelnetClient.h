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

#include "TelnetProtocol.h"

namespace WinTN3270
{
	/***********************************************************
	 Client for communicating with a remote TELNET server.
	***********************************************************/
	public ref class CTelnetClient
	{

	public: /* Public Delegates */
		delegate bool OnCertPolicyError( 
			System::Security::Cryptography::X509Certificates::X509Certificate^ mpCertificate, 
			System::Security::Cryptography::X509Certificates::X509Chain^ mpChain, 
			System::Net::Security::SslPolicyErrors ePolicyErrors);
		delegate void OnReceiveASCII(System::String^ cchData);
		delegate void OnReceiveBinary(cli::array<System::Byte>^ mpData);
		delegate void OnSend();

	private: /* Private Member Attributes */
		System::String^ m_cchRemoteHost;
		bool m_fGoAhead;
		OnCertPolicyError^ m_mpCertErrorCbk;
		System::Collections::Hashtable^ m_mpClientOpts;
		System::Threading::EventWaitHandle^ m_mpSendEvent;
		cli::array<System::Byte>^ m_mpRcvCommandBuffer;
		cli::array<System::Byte>^ m_mpRcvDataBuffer;
		OnReceiveASCII^ m_mpReceiveAscCbk;
		OnReceiveBinary^ m_mpReceiveBinCbk;
		System::Net::IPEndPoint^ m_mpRemoteIP;
		OnSend^ m_mpSendCbk;
		System::Collections::Hashtable^ m_mpServerOpts;
		System::Net::Sockets::Socket^ m_mpSocket;
		System::IO::Stream^ m_mpSocketStream;
		TNSUBOPTION^ m_mpSubOpt;
		cli::array<System::Byte>^ m_mpTransmQueue;

	public: /* Public Properties */
		property bool BinaryTransmission
		{
			bool get() 
			{ 
				return m_mpClientOpts->ContainsKey(safe_cast<int^>(TELNET_OPTION_BINARY));
			}

			void set(bool fValue)
			{ 
				if (!fValue && 
					m_mpClientOpts->ContainsKey(safe_cast<int^>(TELNET_OPTION_BINARY)))
				{
					m_mpClientOpts->Remove(safe_cast<int^>(TELNET_OPTION_BINARY));
				}

				m_mpClientOpts[safe_cast<int^>(TELNET_OPTION_BINARY)] = true;
			}
		}

		property OnCertPolicyError^ CertificateErrorCallback
		{
			OnCertPolicyError^ get()
			{
				System::Threading::Monitor::Enter(this);
				OnCertPolicyError^ mpCertErrorCbk = m_mpCertErrorCbk;
				System::Threading::Monitor::Exit(this);

				return mpCertErrorCbk;
			}

			void set(OnCertPolicyError^ mpValue)
			{ 
				System::Threading::Monitor::Enter(this);
				m_mpCertErrorCbk = mpValue;
				System::Threading::Monitor::Exit(this);
			}
		}

		property bool Connected
		{
			bool get() { return m_mpSocket->Connected; }
		}

		property bool EndOfRecord
		{
			bool get() 
			{
				return m_mpClientOpts->ContainsKey(TELNET_OPTION_END_OF_RECORD);
			}

			void set(bool fValue)
			{
				if (!fValue && 
					m_mpClientOpts->ContainsKey(TELNET_OPTION_END_OF_RECORD))
				{
					m_mpClientOpts->Remove(TELNET_OPTION_END_OF_RECORD);
					return;
				}

				m_mpClientOpts[TELNET_OPTION_END_OF_RECORD] = true;
			}
		}

		property bool FullDuplex
		{
			bool get() 
			{
				return m_mpClientOpts->ContainsKey(TELNET_OPTION_SUPPRESS_GA);
			}

			void set(bool fValue)
			{
				if (!fValue && 
					m_mpClientOpts->ContainsKey(TELNET_OPTION_SUPPRESS_GA))
				{
					m_mpClientOpts->Remove(TELNET_OPTION_SUPPRESS_GA);
					return;
				}

				m_mpClientOpts[TELNET_OPTION_SUPPRESS_GA] = true;
			}
		}

		property OnReceiveASCII^ ReceiveCallbackASCII
		{
			OnReceiveASCII^ get() { return m_mpReceiveAscCbk; }

			void set(OnReceiveASCII^ mpValue)
			{
				m_mpReceiveAscCbk = mpValue;
			}
		}

		property OnReceiveBinary^ ReceiveCallbackBinary
		{
			OnReceiveBinary^ get() { return m_mpReceiveBinCbk; }

			void set(OnReceiveBinary^ mpValue) 
			{
				m_mpReceiveBinCbk = mpValue;
			}
		}

		property OnSend^ SendCallback
		{
			OnSend^ get() { return m_mpSendCbk; }

			void set(OnSend^ mpValue) 
			{
				m_mpSendCbk = mpValue;
			}
		}

		property cli::array<System::Byte>^ TerminalType
		{
			cli::array<System::Byte>^ get()
			{
				if (!m_mpClientOpts->ContainsKey(TELNET_OPTION_TERM_TYPE)) 
					return nullptr;

				return (cli::array<System::Byte>^)m_mpClientOpts[TELNET_OPTION_TERM_TYPE];
			}

			void set(cli::array<System::Byte>^ mpValue)
			{
				if (mpValue == nullptr && 
					m_mpClientOpts->ContainsKey(TELNET_OPTION_TERM_TYPE))
				{
					m_mpClientOpts->Remove(TELNET_OPTION_TERM_TYPE);
					return;
				}

				m_mpClientOpts[TELNET_OPTION_TERM_TYPE] = mpValue;
			}
		}

	private: /* Private Member Functions */
		bool CertValidationCallback( System::Object^ mpSender, 
			System::Security::Cryptography::X509Certificates::X509Certificate^ mpCertificate, 
			System::Security::Cryptography::X509Certificates::X509Chain^ mpChain, 
			System::Net::Security::SslPolicyErrors ePolicyErrors);
		void FlushTransmitQueue();
		cli::array<System::Byte>^ GetClientOption(int nId);
		cli::array<System::Byte>^ InterpretCommands(cli::array<System::Byte>^ mpData);
		static void ReceiveProc(System::IAsyncResult^ mpResult);
		static void SendProc(System::IAsyncResult^ mpResult);
		bool SetServerOption(int nId, bool fValue);
		bool SetServerOption(int nId, cli::array<System::Byte>^ mpValue);
		void Transmit(cli::array<System::Byte>^ mpData);

	public: /* Public Member Functions */
		CTelnetClient(System::String^ cchIPAddr, int nPort);
		~CTelnetClient();
		void Connect();
		void Connect(bool fSecure);
		void Disconnect();
		cli::array<System::Byte>^ GetServerOption(int nId);
		void Send(System::String^ cchData);
		void Send(cli::array<System::Byte>^ mpData);
		bool TestServerOption(int nId);
	};

	/* Socket Receive State */
	ref struct SOCKETRCVSTATE
	{
		cli::array<System::Byte>^ mpBuffer;
		CTelnetClient::OnReceiveASCII^ mpAscCallback;
		CTelnetClient::OnReceiveBinary^ mpBinCallback;
		CTelnetClient^ mpClient;
		System::IO::Stream^ mpStream;
	};

	/* Socket Send State */
	ref struct SOCKETSNDSTATE
	{
		CTelnetClient::OnSend^ mpCallback;
		CTelnetClient^ mpClient;
		System::IO::Stream^ mpStream;
	};
}
