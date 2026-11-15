//---------------------------------------------------------------------------

#include <vcl.h>
#include <map>
#include <regex>
#pragma hdrstop

#include "Client_u.h"
#include <common/enigma.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm2 *Form2;
time_t wtime;
struct tm tm_w = { 0 };
int sec, min, tics, CurSlot, oldsec, diviz;
TPoint lineStartPos, lineEndPos;
Graphics::TBitmap * bmp = new Graphics::TBitmap();
Enigma_cry  Enigma;

std::map<AnsiString, AnsiString> tranlit;
std::map<AnsiString, AnsiString> rev_tranlit;
char rus_mas[] = {'а','б','ц','д','е','ё','ф','г','х','и','й','к','л','м','н','о','п','ф','р','с','т','у','в','ш','ж','я','з','ч','щ','ь','ъ','ы','э','ю'};
AnsiString eng_mas[] = {"a","b","c","d","e","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z","vj","ww","gj","dj","xj","bj","jj"};

//---------------------------------------------------------------------------
__fastcall TForm2::TForm2(TComponent* Owner): TForm(Owner) {
	HWND hWnd = FindWindow(NULL, L"Сервер звука");
	if (hWnd == NULL) {
		ShellExecute (NULL, NULL, L"player_c.exe", L"", NULL, SW_NORMAL);
		Sleep(1000);
	}
	hWnd = FindWindow(NULL, L"Сервер звука");
	if (hWnd == NULL) {
		MessageDlg("нет сервера звука", mtError, TMsgDlgButtons() << mbOK, 0);
        Application->Terminate();
		exit;
	}

	DevCB->ItemIndex = 3;
	DevCBChange(this);
	txtMessageChange(Owner);
	bmp=ft8_IC->GetBitmap(0,105,105);
	int n = sizeof(rus_mas)/sizeof(char);
	for (int i = 0; i < n; i++)
		tranlit[rus_mas[i]] = eng_mas[i];
	for(std::map <AnsiString, AnsiString>::iterator it = tranlit.begin(); it != tranlit.end(); it++)
		rev_tranlit[(*it).second] = (*it).first;
}

AnsiString __fastcall TForm2::Translit(AnsiString Source) {
	String s_o = "";
	AnsiString Result, e;

	for(int i = 0; i < Source.Length(); i++) {
		e = tranlit[Source[i + 1]];
		if (e.Length() > 0) {
			Result += tranlit[Source[i + 1]];
		} else {
			Result += Source[i + 1];
		}
	}
	return Result.LowerCase();
}

//---------------------------------------------------------------------------
void __fastcall TForm2::SendMes() {
	String ParamString;

	if (Trim(TransMemo->Text).Length()==0) {
		MessageDlg("нет текста для передачи", mtError, TMsgDlgButtons() << mbOK, 0);
		exit;
	}
                                      // 6 - 'это команда для выдачи информации для звука'
	ParamString = Format("%d%d%2d%2d%s", 6, 2, 0, diviz, TransMemo->Text);

	try {
		IdTCPClient1->Host = txtServer->Text;
		try {
			if(!IdTCPClient1->Connected() ) IdTCPClient1->Connect();

			IdTCPClient1->Socket->WriteLn(ParamString, enUTF8);
			txtResults->Lines->Add(IdTCPClient1->Socket->ReadLn(enUTF8));
		} catch (...) {
			MessageDlg("нет соединения с сервером", mtError, TMsgDlgButtons() << mbOK, 0);
		}
	}
	__finally {
		 IdTCPClient1->Disconnect();
	}
}


void __fastcall TForm2::DevCBChange(TObject *Sender) {
try {
		IdTCPClient1->Host = txtServer->Text;
		try {
			if(!IdTCPClient1->Connected() )	IdTCPClient1->Connect();
			String s = StringReplace("9"+DevCB->Text, "\r\n", "", TReplaceFlags() << rfReplaceAll);
			IdTCPClient1->Socket->WriteLn(s, enUTF8);
			txtResults->Lines->Add(IdTCPClient1->Socket->ReadLn(enUTF8));
		} catch (...) {
			MessageDlg("нет соединения с сервером", mtError, TMsgDlgButtons() << mbOK, 0);
			Application->Terminate();
		}

	}
	__finally {
		 IdTCPClient1->Disconnect();
	}
}
//---------------------------------------------------------------------------


void __fastcall TForm2::txtMessageChange(TObject *Sender) {
	CountEdit->Text = txtMessage->Text.Length();
}
//---------------------------------------------------------------------------

void __fastcall TForm2::WatchTimerTimer(TObject *Sender) {
	wtime = time(NULL);
	localtime_r(&wtime, &tm_w);
	sec = tm_w.tm_sec;
	min = tm_w.tm_min;

	if (oldsec!=sec) {
		oldsec = sec;
		if (sec<=14)	   CurSlot = 1;
		else if (sec<=29)  CurSlot = 2;
		else if (sec<=44)  CurSlot = 3;
		else 			   CurSlot = 4;
		lineStartPos = TPoint();
		lineEndPos = TPoint();
		WatchPanel->Invalidate();
		lineStartPos = TPoint(WatchPanel->Width/2, WatchPanel->Height/2);
		lineEndPos = TPoint(WatchPanel->Width/2 * (1 + sin(sec * 3.1415/30)), WatchPanel->Width/2 * (1 - cos(sec * 3.1415/30)));
		WatchPanel->Invalidate();

		if ((sec==0) || (sec==15) || (sec==30) || (sec==45)) {
LARGE_INTEGER EndTime, StartTime;
int t;
__int64 CounterStart = 0;
QueryPerformanceCounter(&StartTime);
CounterStart = StartTime.QuadPart;

		diviz = (sec==0) ? 2 : sec-9;
		SendMes();

QueryPerformanceCounter(&EndTime);
t = (EndTime.QuadPart - CounterStart) / 10000;
Edit1->Text = IntToStr(t) + " msec";
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm2::BButtonClick(TObject *Sender) {
	if (Trim(TransMemo->Text).Length()==0)
		MessageDlg("нет текста для передачи", mtError, TMsgDlgButtons() << mbOK, 0);
	else {
		WatchTimer->Enabled = True;
		BButton->Enabled = False;
		EButton->Enabled = True;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm2::EButtonClick(TObject *Sender) {
	WatchTimer->Enabled = False;
	BButton->Enabled = True;
	EButton->Enabled = False;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::WatchPanelPaint(TObject *Sender) {
	if (!lineStartPos.IsEmpty() && !lineEndPos.IsEmpty()) {
		WatchPanel->Canvas->Draw(0, 0, bmp);
		WatchPanel->Canvas->MoveTo(lineStartPos.x, lineStartPos.y);
		WatchPanel->Canvas->LineTo(lineEndPos.x, lineEndPos.y);
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm2::TextBtnClick(TObject *Sender) {
	txtMessage->Text = "Схема та!@#$%^& же, что в ор|и/гин^а()льн{}ой ста`тье, но с указанием конкретных номиналов компонентов, способа изготовления устройства и его настройки. T1 представляет собой 35 витков тонкого эмалированного провода на кольце T68-2. В это кольцо продевается отрезок коаксиального кабеля.";
	String s = txtMessage->Text;
	char buff[500];
	wsprintfA(buff, "%ls", s.c_str());
	buff[499] = '\0';
	s=regex_replace(buff, std::regex("[`'\"!<>@#%_+()\\^$&/{}*|]"), "");
	txtMessage->Text = s;
	RealEdit->Text = "";
	TransMemo->Text = "";
}
//---------------------------------------------------------------------------

void __fastcall TForm2::DecryptBtnClick(TObject *Sender) {
	if (Trim(TransMemo->Text).Length()>0) {
		TransMemo->Text = Enigma.Decrypt(TransMemo->Text);
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::EncryptBtnClick(TObject *Sender) {
	if (Trim(TransMemo->Text).Length()>0) {
		TransMemo->Text = Enigma.Encrypt(TransMemo->Text);
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ToTranslitBtnClick(TObject *Sender) {
	String s = Trim(txtMessage->Text);
	char buff[400];
	wsprintfA(buff, "%.500ls", s.c_str());
	buff[399] = '\0';
	std::string xStr = buff;
	xStr=regex_replace(xStr, std::regex("[-]"), " ");
	xStr=regex_replace(xStr, std::regex("[,]"), ".");
	s=regex_replace(xStr, std::regex("[`'\"!<>@#%_+()\\^$&/{}*|]"), "");

	s = Translit(s.LowerCase());
	RealEdit->Text = IntToStr(s.Length());
	TransMemo->Text = s;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::FormDestroy(TObject *Sender) {
	ShellExecute (NULL, NULL, L"taskkill", L"/im player_c.exe /f", NULL, SW_HIDE);
}
//---------------------------------------------------------------------------

