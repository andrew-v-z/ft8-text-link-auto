//---------------------------------------------------------------------------

#ifndef Client_uH
#define Client_uH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <Vcl.BaseImageCollection.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.ImageCollection.hpp>
//---------------------------------------------------------------------------
class TForm2 : public TForm
{
__published:	// IDE-managed Components
	TMemo *txtResults;
	TEdit *txtServer;
	TIdTCPClient *IdTCPClient1;
	TLabel *Label1;
	TMemo *txtMessage;
	TComboBox *DevCB;
	TLabel *Label2;
	TEdit *CountEdit;
	TLabel *Label3;
	TImageCollection *ft8_IC;
	TPaintBox *WatchPanel;
	TTimer *WatchTimer;
	TEdit *Edit1;
	TButton *BButton;
	TButton *EButton;
	TButton *TextBtn;
	TButton *EncryptBtn;
	TButton *DecryptBtn;
	TButton *ToTranslitBtn;
	TLabel *Label4;
	TEdit *RealEdit;
	TMemo *TransMemo;
	void __fastcall SendMes();
	void __fastcall DevCBChange(TObject *Sender);
	void __fastcall txtMessageChange(TObject *Sender);
	void __fastcall WatchTimerTimer(TObject *Sender);
	void __fastcall BButtonClick(TObject *Sender);
	void __fastcall EButtonClick(TObject *Sender);
	void __fastcall WatchPanelPaint(TObject *Sender);
	void __fastcall TextBtnClick(TObject *Sender);
	void __fastcall DecryptBtnClick(TObject *Sender);
	void __fastcall EncryptBtnClick(TObject *Sender);
	void __fastcall ToTranslitBtnClick(TObject *Sender);
	AnsiString __fastcall Translit(AnsiString Source);
	void __fastcall FormDestroy(TObject *Sender);

private:	// User declarations
public:		// User declarations
	__fastcall TForm2(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm2 *Form2;
//---------------------------------------------------------------------------
#endif
