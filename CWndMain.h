#pragma once

#include "CClient3270.h"

namespace WinTN3270 {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class CWndMain : public System::Windows::Forms::Form
	{
	public:
		CWndMain(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~CWndMain()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::MenuStrip^  mnuMain;
	private: System::Windows::Forms::StatusStrip^  sbrMain;
	protected: 

	protected: 

	private: System::Windows::Forms::ToolStrip^  tlbMain;
	private: System::Windows::Forms::PictureBox^  pbxCanvas;




	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->mnuMain = (gcnew System::Windows::Forms::MenuStrip());
			this->sbrMain = (gcnew System::Windows::Forms::StatusStrip());
			this->tlbMain = (gcnew System::Windows::Forms::ToolStrip());
			this->pbxCanvas = (gcnew System::Windows::Forms::PictureBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pbxCanvas))->BeginInit();
			this->SuspendLayout();
			// 
			// mnuMain
			// 
			this->mnuMain->Location = System::Drawing::Point(0, 0);
			this->mnuMain->Name = L"mnuMain";
			this->mnuMain->Size = System::Drawing::Size(817, 24);
			this->mnuMain->TabIndex = 0;
			this->mnuMain->Text = L"menuStrip1";
			// 
			// sbrMain
			// 
			this->sbrMain->Location = System::Drawing::Point(0, 539);
			this->sbrMain->Name = L"sbrMain";
			this->sbrMain->Size = System::Drawing::Size(817, 22);
			this->sbrMain->TabIndex = 1;
			this->sbrMain->Text = L"statusStrip1";
			// 
			// tlbMain
			// 
			this->tlbMain->Location = System::Drawing::Point(0, 24);
			this->tlbMain->Name = L"tlbMain";
			this->tlbMain->Size = System::Drawing::Size(817, 25);
			this->tlbMain->TabIndex = 2;
			this->tlbMain->Text = L"toolStrip1";
			// 
			// pbxCanvas
			// 
			this->pbxCanvas->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->pbxCanvas->Location = System::Drawing::Point(0, 51);
			this->pbxCanvas->Name = L"pbxCanvas";
			this->pbxCanvas->Size = System::Drawing::Size(817, 485);
			this->pbxCanvas->TabIndex = 3;
			this->pbxCanvas->TabStop = false;
			// 
			// CWndMain
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(817, 561);
			this->Controls->Add(this->pbxCanvas);
			this->Controls->Add(this->tlbMain);
			this->Controls->Add(this->sbrMain);
			this->Controls->Add(this->mnuMain);
			this->MainMenuStrip = this->mnuMain;
			this->Name = L"CWndMain";
			this->Text = L"CWndMain";
			this->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &CWndMain::CWndMain_KeyPress);
			this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &CWndMain::CWndMain_KeyDown);
			this->Load += gcnew System::EventHandler(this, &CWndMain::CWndMain_Load);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->pbxCanvas))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	private: /* Private Member Attributes */
		CClient3270^ m_mpClient;

	private: /* Private Member Functions */
		System::Void CWndMain_Load(System::Object^ sender, System::EventArgs^ e);
		System::Void CWndMain_KeyDown(System::Object^ sender, 
			System::Windows::Forms::KeyEventArgs^ e);
		System::Void CWndMain_KeyPress(System::Object^ sender, 
			System::Windows::Forms::KeyPressEventArgs^ e);
		bool m_mpClient_CertPolicyError( 
			System::Security::Cryptography::X509Certificates::X509Certificate^ mpCertificate, 
			System::Security::Cryptography::X509Certificates::X509Chain^ mpChain, 
			System::Net::Security::SslPolicyErrors ePolicyErrors);

	protected: /* Overrides */
		virtual void OnPaint(PaintEventArgs^ e) override;
};
}

