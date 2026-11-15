//---------------------------------------------------------------------------

#ifndef FT8transportH
#define FT8transportH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.MPlayer.hpp>
#include <System.Actions.hpp>
#include <Vcl.ActnList.hpp>
#include <Vcl.Grids.hpp>
#include <Vcl.Menus.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdContext.hpp>
#include <IdCustomTCPServer.hpp>
#include <IdTCPServer.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <System.ImageList.hpp>
#include <Vcl.BaseImageCollection.hpp>
#include <Vcl.ImageCollection.hpp>
#include <Vcl.ImgList.hpp>

//---------------------------------------------------------------------------
class TForm1 : public TForm {
__published:	// IDE-managed Components
	TButton *UnScrriptBtn;
	TMemo *Memo1;
	TFileOpenDialog *FileOpenDialog1;
	TEdit *mesEdit;
	TEdit *toEdit;
	TButton *AuBtn;
	TProgressBar *ProgressBar1;
	TButton *ReceiveBtn;
	TComboBox *InputsCB;
	TComboBox *OutputsCB;
	TLabel *Label1;
	TLabel *Label2;
	TRadioGroup *ModeNameRG;
	TEdit *NameEdit;
	TEdit *NameIndexEdit;
	TActionList *ActionList1;
	TAction *CloseAction;
	TStringGrid *DivisionNameSG;
	TTimer *WatchTimer;
	TPaintBox *WatchPanel;
	TButton *PlayNewBtn;
	TButton *TextBtn;
	TButton *ToTranslitBtn;
	TPaintBox *InPB;
	TTrackBar *InBar;
	TEdit *Edit1;
	TTrackBar *OutBar;
	TPaintBox *OutPB;
	TButton *EncryptBtn;
	TButton *DecryptBtn;
	TMainMenu *ProgMainMenu;
	TMenuItem *MainMenu;
	TMenuItem *SyncTimeMenu;
	TMenuItem *SNRMenu;
	TButton *MessToBufferBtn;
	TListBox *TransLB;
	TPopupMenu *MessagesPopupMenu;
	TMenuItem *PopUpMenuShowMess;
	TButton *NextFromBufBtn;
	TButton *ClearBtn;
	TIdTCPClient *TCPClient;
	TImage *Image1;
	TEdit *QQEdit;
	TLabel *Label_qq;
	TImageCollection *ft8_IC;
	TButton *Button1;
	void __fastcall UnScriptBtnClick(TObject *Sender);
	void __fastcall AuBtnClick(TObject *Sender);
	void __fastcall Documentator(char * l, int div, int in_out);
	void __fastcall ReceiveBtnClick(TObject *Sender);
	int __fastcall  RealDevice(int in_out);
	void __fastcall ModeNameRGClick(TObject *Sender);
	void __fastcall NameIndexEditChange(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall CloseActionExecute(TObject *Sender);
	void __fastcall DivisionNameSGDrawCell(TObject *Sender, System::LongInt ACol, System::LongInt ARow,
		  TRect &Rect, TGridDrawState State);
	void __fastcall PaintRGreed(void);
	void __fastcall MakeMessage(uint8_t mes_type, uint8_t adresat, char * original);
	void __fastcall WatchTimerTimer(TObject *Sender);
	void __fastcall WatchPanelPaint(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall ThreadsClose(void);
	void __fastcall AllColLen(TStringGrid * G);
	void __fastcall PlayNewBtnClick(TObject *Sender);
	void __fastcall TextBtnClick(TObject *Sender);
	void __fastcall ToTranslitBtnClick(TObject *Sender);
	void __fastcall PlayOut(void);
	void __fastcall SetDevice(void);

	AnsiString __fastcall Translit(AnsiString Source);
	AnsiString __fastcall rev_Translit(AnsiString Source);
	void __fastcall InBarChange(TObject *Sender);
	void __fastcall InPBPaint(TObject *Sender);
	void __fastcall FormPaint(TObject *Sender);
	void __fastcall OutBarChange(TObject *Sender);
	void __fastcall OutPBPaint(TObject *Sender);
	void __fastcall Obrab_Thread();

	void __fastcall EncryptBtnClick(TObject *Sender);
	void __fastcall DecryptBtnClick(TObject *Sender);
	void __fastcall SyncTimeMenuClick(TObject *Sender);
	void __fastcall SNRMenuClick(TObject *Sender);
	void __fastcall MessToBufferBtnClick(TObject *Sender);
	 int __fastcall Fill_TransiverList(bool array);
	void __fastcall PopUpMenuShowMessClick(TObject *Sender);
	void __fastcall TransLBDblClick(TObject *Sender);
	void __fastcall NextFromBufBtnClick(TObject *Sender);
	void __fastcall ClearBtnClick(TObject *Sender);
	bool __fastcall NeedPause ( void );
	void __fastcall OutputsCBChange(TObject *Sender);
	void __fastcall Button1Click(TObject *Sender);

private:	// User declarations
public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
