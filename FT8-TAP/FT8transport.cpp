//---------------------------------------------------------------------------

#include <shellapi.h>
#include <vcl.h>
#include <map>
#include <regex>
#include <cstring>
#include <DateUtils.hpp>
#include <IniFiles.hpp>
#include <Vcl.Dialogs.hpp>

#include "include/portaudio.h"
#include <common/common.h>
#include <common/t_mess_arr.h>
#include <common/lmq_arr.h>
#include <common/wave.h>
#include <common/monitor.h>
#include <common/audio.h>
#include <common/soundthread.h>
#include <common/timethread.h>
//#include <common/enigma.h>
#include <ft8/text.h>

#pragma hdrstop
#include "FT8transport.h"
/*
#define MAX_MESS_QNT 32
#define COMMON_CALL 63
#define MAIN_KNOT 0
#define BEG_FREQ 400.0
#define ABONENTS_MAX 15
#define BAND_USED 2300
#define TF8_BAND_FREQ 72
#define TOTAL_DIVISIONS 43
*/
extern "C" void decode(const monitor_t* mon, s_attachment * letter_out);
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TForm1 *Form1;
HANDLE mutex;
int INvolume, OUTvolume;
SoundVolume InVol, OutVol;
//Enigma_cry  Enigma;
float alevelIN, alevelOUT, BAND_FREQ = (float)BAND_USED/ABONENTS_MAX;
IAudioMeterInformation *pMeterInfoIN, *pMeterInfoOUT;

char f[400];
char letter[400] = {0};

monitor_t mon_Main;

int division, LongMessTransmitter=5;
bool Transmiting, Silense_local, Silense_Staff, stay_mes_in_buf = false, ModeSNR=false;
TIniFile *IniFile;
int InputMesType, TransmitSlot, CurSlot, MemSlot;
TPoint lineStartPos, lineEndPos;
struct tm tm_w = { 0 };
time_t wtime;
int sec, oldsec, tics, min, Mode;
String Path_Messages_In, Path_Messages_Out;
Graphics::TBitmap * bmp = new Graphics::TBitmap();
DynamicArray1<t_mess> ddd;
LongMessQueue<t_lmq>  lmq;

String All_Divisions[64] = {
"Dot0","Dot1","Dot2","Dot3","Dot4","Dot5","Dot6","Dot7","Dot8","Dot9",
"Dot10","Dot11","Dot12","Dot13","Dot14","Dot15","Dot16","Dot17","Dot18","Dot19",
"Dot20","Dot21","Dot22","Dot23","Dot24","Dot25","Dot26","Dot27","Dot28","Dot29",
"Dot30","Dot31","Dot32","Dot33","Dot34","Dot35","Dot36","Dot37","Dot38","Dot39",
"Dot40","Dot41","Dot42","Dot43","","","","","","",
"","","","","","","","","","",
"2 слот","3 слот","4 слот","Общий"
};
String Mes_Types[5] = {"Сигнал/шум","Простое","Длинное","Тишина в эфире","Синхронизация",};

std::map<AnsiString, AnsiString> tranlit;
std::map<AnsiString, AnsiString> rev_tranlit;
char rus_mas[] =       {'а','б','ц','д','е','ё','ф','г','х','и','й','к','л','м','н','о','п','р','с','т','у','в','ш','ж','я','з','ч','щ','ь','ъ','ы','э','ю'};
AnsiString eng_mas[] = {"a","b","c","d","e","e","f","g","h","i","j","k","l","m","n","o","p","r","s","t","u","v","w","x","y","z","q","vj","gj","dj","xj","bj","jj"};

String SlashSep(String Path, String FName) {
	if (Path.SubString(Path.Length(), 1) != "\\")
		return(Path + "\\" + FName);
	else return(Path + FName);
}

void RemoveAll(String path) {
    TSearchRec sr;
    if (FindFirst(path + "\\*.*", faAnyFile, sr) == 0) {
        do {
            if ((sr.Attr & faDirectory) == 0) {
                DeleteFile(path + "\\" + sr.Name);
            } else if (Pos('.', sr.Name) <= 0)
                RemoveAll(path + "\\" + sr.Name);
        } while (FindNext(sr) == 0);
    }
    FindClose(sr);
}

//---------------------------------------------------------------------------
int __fastcall TForm1::RealDevice(int in_out) {
	int b;
	if (in_out==0) {
		b = (InputsCB->Items->Strings[InputsCB->ItemIndex]).Pos('.');
		return InputsCB->Items->Strings[InputsCB->ItemIndex].SubString(1,b-1).ToInt()-1;
	} else {
		b = (OutputsCB->Items->Strings[OutputsCB->ItemIndex]).Pos('.');
		return OutputsCB->Items->Strings[OutputsCB->ItemIndex].SubString(1,b-1).ToInt()-1;
	}
}
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner): TForm(Owner) {
	PaError pa_rc;
	char original[200];
	int numDevices, i;
	PaStreamParameters inputParameters;
	PaStreamParameters outputParameters;
	double sample_rate;

/*	HWND hWnd = FindWindow(NULL, L"Система шифрованной связи МЧС Татарстан");
	if (hWnd != NULL) {
		Application->Terminate();
		exit (0);
	}
*/
	mutex=CreateMutex(NULL,true,L"MutexOfTheMyApplication");
	DWORD result;
	result = WaitForSingleObject(mutex,0);//А теперь пытаемся его захватить себе
	//Если его захватить не получается, значит мютекс уже захватил кто-то другой
	if(result!=WAIT_OBJECT_0) {
		Application->ShowMainForm=false;
		Application->Terminate();
	}

	Memo1->Lines->Clear();
	char buff[400];
	pa_rc = Pa_Initialize(); // Initialize PortAudio
	if (pa_rc != paNoError) {
		Memo1->Lines->Add("Error initializing PortAudio.");
		sprintf(original,"Errortext: %s Number: %d", Pa_GetErrorText(pa_rc), pa_rc);
		Memo1->Lines->Add(original);
		Pa_Terminate(); // I don't think we need this but...
	} else
		Memo1->Lines->Add("PortAudio initialized.");

	numDevices = Pa_GetDeviceCount();
	if (numDevices < 0) {
		sprintf(original,"ERROR: Pa_CountDevices returned 0x%x", numDevices);
		Memo1->Lines->Add(original);
		return;
	}

	sprintf(original,"%d audio devices found:", numDevices);
	Memo1->Lines->Add(original);

	for ( i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);

		inputParameters.device = i,
		inputParameters.channelCount = 1, // 1 = mono, 2 = stereo
		inputParameters.sampleFormat = paFloat32,
		inputParameters.suggestedLatency = 0.2,
		inputParameters.hostApiSpecificStreamInfo = NULL;

		sample_rate = 12000; // sample rate (frames per second)
		pa_rc = Pa_IsFormatSupported(&inputParameters, NULL, sample_rate);

		char buff[256];
		wsprintfA(buff, "%ls",(UTF8ToString((UnicodeString)(deviceInfo->name))).c_str());
		sprintf(original,"%d. %s [%s]", i+1, buff, (pa_rc == paNoError) ? "OK" : "NOT SUPPORTED");
		if (pa_rc == paNoError)
			InputsCB->Items->Add(original);
	}
	InputsCB->ItemIndex = 1;

	for ( i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);

		outputParameters.device = i,
		outputParameters.channelCount = 1, // 1 = mono, 2 = stereo
		outputParameters.sampleFormat = paFloat32,
		outputParameters.suggestedLatency = 0.2,
		outputParameters.hostApiSpecificStreamInfo = NULL;

		sample_rate = 12000; // sample rate (frames per second)
		pa_rc = Pa_IsFormatSupported( NULL, &outputParameters, sample_rate);

		char buff[256];
		wsprintfA(buff, "%ls",(UTF8ToString((UnicodeString)(deviceInfo->name))).c_str());
		sprintf(original,"%d. %s [%s]", i+1, buff, (pa_rc == paNoError) ? "OK" : "NOT SUPPORTED");
		if (pa_rc == paNoError)
			OutputsCB->Items->Add(original);
	}
	OutputsCB->ItemIndex = 1;

	Pa_Terminate();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UnScriptBtnClick(TObject *Sender) {
	char buff[256];
	if (FileOpenDialog1->Execute()) {
		wsprintfA(buff,"%ls",FileOpenDialog1->FileName.c_str());
		Memo1->Lines->Add(buff);
		strcpy(f, buff);
	}

	if (strlen(f)>0) {
		float snr=0, ans_freq=0, db=0;;
		int i3=-1;

#ifdef ENG_VARIANT
		String s = LowerCase(decode_ft8(f, &i3, &snr, &ans_freq));
#else
		String s = rev_Translit(LowerCase(decode_ft8(f, &i3, &snr, &ans_freq)));
#endif
		sprintf(buff, " (%.1f dB  %.1f Hz)", 33.1-snr, ans_freq);
		Memo1->Lines->Add(s + buff);
		Memo1->Lines->Add("div_relative=" + IntToStr(int(round((ans_freq-BEG_FREQ)/BAND_FREQ)))  // 15 numbers only
								 + "; i3=" + IntToStr(i3));
	}
}
//-------------------------------------------MakeMessage--------------------------------
int FindFreqForDivision(int mode) {
	if (division==MAIN_KNOT)
		return BEG_FREQ;
	else if(mode==MESSAGE_LONG)
		return (BEG_FREQ);
	else
		return (BEG_FREQ + BAND_FREQ * (division-(TransmitSlot-2)*ABONENTS_MAX));  // 15 numbers only
}
//---------------------------------------------------------------------------

void __fastcall TForm1::MakeMessage(uint8_t mes_type, uint8_t adresat, char * original) {
	int count = 0, l;                            // в original уже зашифрованное сообщение
	bool buf_empty;
	char zapros[2] = {'S','\0'};
	char temp[256];

	if ((mes_type==MESSAGE_LONG) && (TransmitSlot>1)) {  // для подразделений
														 // если длинное, то сначала создаём запрос тишины в штаб;
		buf_empty = (ddd.size()==0);              // проверяем, надо ли создавать файл

//		ddd.DynArrayAdd( MESSAGE_SILENSE, MAIN_KNOT, zapros); // вставляем запрос тишины к главному в интерфейсах подразделений
		if (buf_empty) {

/*LARGE_INTEGER EndTime, StartTime;
int t;
__int64 CounterStart = 0;
QueryPerformanceCounter(&StartTime);
CounterStart = StartTime.QuadPart;

QueryPerformanceCounter(&EndTime);
t = (EndTime.QuadPart - CounterStart) / 10000;
Edit1->Text = IntToStr(t) + " msec"; */

//			Mode = MESSAGE_SILENSE;                    	// меняем режим на режим тишины перед передачей запроса тишины
Memo1->Lines->Add("Mode_mm+="+IntToStr(Mode));
		}
	}  													// для главного узла ничего не меняем

	l = strlen(original);

	Memo1->Lines->Add("Сообщение зашифровано.");
	sprintf(temp, "Длина сообщения: %d\n", l);
	Memo1->Lines->Add(temp);

	count = l / 12;
	count = count + ((l % 12>0) ? 1 : 0);

	buf_empty = (ddd.size()==0);              // проверяем, надо ли создавать файл
	ddd.DynArrayAdd( mes_type, adresat, original);

	if (buf_empty)  // значит, не MESSAGE_LONG в подразделении  или  в буфере уже было сообщение
		Mode = mes_type;

	Fill_TransiverList(true); // заполнение листбокса
	Memo1->Lines->Add("Частей " + IntToStr(count+1));
Memo1->Lines->Add("Mode_mm+="+IntToStr(Mode));

//	sprintf(temp, "%s", ".");
//	gen_ft8_hor_to_file( adresat, mes_type, original, temp);

	return;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::NextFromBufBtnClick(TObject *Sender) {    // для удаления старого и извлечения нового сообщения из буфера
	if (stay_mes_in_buf) {                                      // флаг для задержки извлечения на 1 минуту
		stay_mes_in_buf = false;
Memo1->Lines->Add("NextMess_NextFrom="+IntToStr(stay_mes_in_buf));
		return;
	}

	if (ddd.size()>0)	ddd.erase(0);   			// стираем верхнее сообщение

	Fill_TransiverList(true);  						// заполнение листбокса
	if (ddd.size()>0) {        						// в буфере есть сообщения для отправки

		Mode = ddd.operator [](0).type;  			// следующее в буфере

Memo1->Lines->Add("Mode_NextFrom="+IntToStr(Mode));
	} else {                   						// в буфере нет сообщений для отправки
		if (division!=MAIN_KNOT) {                  // у ПОДРАЗДЕЛЕНИЙ
			Silense_local = false;
			Memo1->Lines->Add("Wait_NextFrom="+IntToStr(Silense_local));
		}
		if (Mode!=MESSAGE_REQ_SNR) 	Mode = MESSAGE_UNKNOWN;
Memo1->Lines->Add("Mode_NextFrom+="+IntToStr(Mode));
	}
}

//---------------------------------------------------------------------------
int __fastcall TForm1::Fill_TransiverList(bool array) {

	TransLB->Items->Clear();   // очистка списка в листбоксе

	int n=0;
	if (ddd.size()>0) {

		do {
			if (array) {
				TransLB->Items->Add(IntToStr(n+1)+'\t' + Mes_Types[ddd.operator[](n).type]
					+ " в " + All_Divisions[ddd.operator[](n).to]);
				TransLB->Refresh();
			}
			n++;
		} while (ddd.size() > n);
	}
	return (n);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::AuBtnClick(TObject *Sender) {
	PaError pa_rc;
	char original[200];
	int numDevices, i;
	PaStreamParameters inputParameters;
	double sample_rate;

	Memo1->Lines->Clear();
	char buff[256];
	pa_rc = Pa_Initialize(); // Initialize PortAudio
	if (pa_rc != paNoError) {
		Memo1->Lines->Add("Error initializing PortAudio.");
		sprintf(original,"Errortext: %s Number: %d", Pa_GetErrorText(pa_rc), pa_rc);
		Memo1->Lines->Add(original);
		Pa_Terminate();
	} else
		Memo1->Lines->Add("PortAudio initialized.");

	numDevices = Pa_GetDeviceCount();
	if (numDevices < 0) {
		sprintf(original,"ERROR: Pa_CountDevices returned 0x%x", numDevices);
		Memo1->Lines->Add(original);
		return;
	}

	sprintf(original,"%d audio devices found:", numDevices);
	Memo1->Lines->Add(original);

	for ( i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);

		inputParameters.device = i,
		inputParameters.channelCount = 1, // 1 = mono, 2 = stereo
		inputParameters.sampleFormat = paFloat32,
		inputParameters.suggestedLatency = 0.2,
		inputParameters.hostApiSpecificStreamInfo = NULL;

		sample_rate = 12000; // sample rate (frames per second)
		pa_rc = Pa_IsFormatSupported(&inputParameters, NULL, sample_rate);

		char buff[256];
		wsprintfA(buff, "%ls",(UTF8ToString((UnicodeString)(deviceInfo->name))).c_str());
		sprintf(original,"%s [%s]", buff, (pa_rc == paNoError) ? "OK" : "NOT SUPPORTED");
		Memo1->Lines->Add(original);
	}

	Pa_Terminate();
}

void __fastcall TForm1::Documentator(char * l, int div, int in_out) {

#ifdef ENG_VARIANT
	String ans = LowerCase(l);
#else
	String ans = rev_Translit(LowerCase(l));
#endif
	Memo1->Lines->Add(ans);

	if (in_out==0) {
		char buff[500];
		String path;
		if (MemSlot == 1)
			path = "\\"+ Now().FormatString("yymmdd_hhnnss_") + All_Divisions[MAIN_KNOT] + ".txt";
		else
			path = "\\"+ Now().FormatString("yymmdd_hhnnss_") + All_Divisions[div] + ".txt";
		wsprintfA(buff, "%ls", path.c_str());

		char mout[256];
		wsprintfA(mout, "%ls", Path_Messages_In.c_str());
		char * p;
		p = (char *)mout;
		p = append_string(p, buff);

		wsprintfA(buff, "%ls", ans.c_str());
		save_file(buff, ans.Length(), mout);
	}
	Memo1->Lines->Add("____________________");
}
bool imReceiver(int to) {
	return ( (to == division) || (to==COMMON_CALL) || (to==int(division/ABONENTS_MAX)+60) );
		// только сообщения к адресату, слоту и общие
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Obrab_Thread(){
	int t;
	__int64 CounterStart = 0;
	LARGE_INTEGER EndTime, StartTime;
	QueryPerformanceCounter(&StartTime);
	CounterStart = StartTime.QuadPart;

	// Decode accumulated data (containing slightly less than a full time slot)
	s_attachment attachments[ATTACHMENTS_MAX] = {0};

	decode(&mon_Main, attachments);

	int num_decoded = 0, st_count_parts = 0, ln_count_parts = 0, sn_count_parts = 0, sl_count_parts = 0;
	char * p, long_letter[ATTACHMENTS_MAX][14] = {0};
	int long_Silense[ATTACHMENTS_MAX] = {0};
	int std_divs[MAX_MESS_QNT] = {0}, diff_adr = 0;
	int snr_divs[MAX_MESS_QNT] = {0};
	float snr_common = 0;
	letter[0] = '\0';
	p = letter;

	while ( (num_decoded<ATTACHMENTS_MAX) && (strlen(attachments[num_decoded].attachment)>0)) {
		int sign = attachments[num_decoded].sign_i3;
		if (! imReceiver(attachments[num_decoded].to)) {
			num_decoded++;
			continue;   // только Нужные сообщения
		}
		switch(sign) {
			case MESSAGE_REQ_SNR: {
				int div = int(round((attachments[num_decoded].mes_freq-BEG_FREQ)/BAND_FREQ));  // 15 numbers only
				snr_divs[sn_count_parts++] = div + ((MemSlot-2)*ABONENTS_MAX);   // заносим его в массив с учетом слота
				break;
			}
			case MESSAGE_STANDART: {
				st_count_parts++;
				if ((attachments[num_decoded].n == 0) && (attachments[num_decoded].mes_freq != 0)) {
					bool pres = false;
					int div = int(round((attachments[num_decoded].mes_freq-BEG_FREQ)/BAND_FREQ));
					div += (MemSlot-2)*ABONENTS_MAX;
					int i_st = 0;
					while (i_st<diff_adr) {
						if (std_divs[i_st]==div) {
							pres = true;            // уже есть этот отправитель
							break;
						}
						i_st++;
					}
					if (!pres) {
						std_divs[diff_adr] = div;   // заносим его в массив
						diff_adr++;
					}
				}
				break;
			}
			case MESSAGE_LONG: {
				strcpy(long_letter[attachments[num_decoded].n], attachments[num_decoded].attachment);
				snr_common += attachments[num_decoded].snr;
				ln_count_parts++;
				break;
			}
			case MESSAGE_SILENSE: {                 // копим в массиве индексы сообщений отправителей
				long_Silense[sl_count_parts] = num_decoded;
				sl_count_parts++;
				break;
			}
		}
		num_decoded++;
	}

	if (st_count_parts>0) {
		int cur_div;
		int div_parts = 0;
		for ( int j = 0; j < diff_adr; j++) {         // перебор всех отправителей из массива
			char short_letter[ATTACHMENTS_MAX][14] = {0};
			letter[0] = '\0';                         // каждый отпрваитель идёт в отдельное письмо
			int i;
			for ( i=0; i<num_decoded; i++) {      // перебор всех принятых сообщений
				snr_common = 0;
				if (int(round((attachments[i].mes_freq-BEG_FREQ)/BAND_FREQ)) == std_divs[j]) { // отделяем сообщения отдельного отправителя
					strcpy(short_letter[attachments[i].n], attachments[i].attachment);
					snr_common += attachments[i].snr; // собираем сообщение из 1 или 2 частей
					div_parts++;
				}
			}
			for ( int i=0; i<ATTACHMENTS_MAX; i++)
				p = append_string(p, short_letter[i]);

			i = strlen(letter);
			for (int j = i; j > 0; j--)
			  if (letter[j-1]==' ') {
				letter[j-1]='\0';
				p--;
			  } else break;
//			Enigma.Decrypt_ptr(letter);

			char text[22] = {'\0'};
			sprintf(text, " (%.1f dB, частей %d)", 33.1 - snr_common/div_parts, div_parts);
			p = append_string(p, text);

			Documentator(letter, std_divs[j], 0);
		}
	}

	if (ln_count_parts>0) {
		int i;
		for ( i=0; i<ATTACHMENTS_MAX; i++)
			p = append_string(p, long_letter[i]);

		i = strlen(letter);
		for (int j = i; j > 0; j--)
		  if (letter[j-1]==' ') {
			letter[j-1]='\0';
            p--;
		  } else break;
//		Enigma.Decrypt_ptr(letter);

		char text[22] = {'\0'};
		sprintf(text, " (%.1f dB, частей %d)", 33.1 -snr_common/ln_count_parts, ln_count_parts);
		p = append_string(p, text);

		Documentator(letter, LongMessTransmitter, 0);
	}

	if (sn_count_parts>0) {              // was SNR
		char * p, buff[256];
		for ( int i=0; i<sn_count_parts; i++) {
			if (division==MAIN_KNOT) {  // answer from  subordinates
				int num_division = snr_divs[i];
				sprintf(buff, "%.1f", (33.1 - attachments[num_division].snr));
				Memo1->Lines->Add(All_Divisions[num_division]+ " " + buff);     //name, snr_out, iii3
				String snr_str =  FormatFloat("0.0 Db", 33.1 - attachments[num_division].snr);
				if (num_division<=ABONENTS_MAX)
					Form1->DivisionNameSG->Cells[1][num_division] = snr_str;
				else if (num_division<=ABONENTS_MAX*2)
					DivisionNameSG->Cells[3][num_division-ABONENTS_MAX] = snr_str;
				else
					DivisionNameSG->Cells[5][num_division-ABONENTS_MAX*2] = snr_str;
			} else {            // report SNR to major from subordinate

				Transmiting = false;

				String CurDivision = All_Divisions[division];
				Memo1->Lines->Add(CurDivision + " принят запрос SNR.");
				sprintf(buff, "%s", "SNRANSWER");
				MakeMessage(MESSAGE_REQ_SNR, MAIN_KNOT, buff);
			}
		}
		if (division==MAIN_KNOT) AllColLen(Form1->DivisionNameSG);
	}

	if (sl_count_parts>0) {

	  if (division==MAIN_KNOT) {                          // у ГЛАВНОГО - все запросы тишины от подразделений
		  for ( int i=0; i<sl_count_parts; i++) {               // запросы тишины в текущем таймслоте
																// за минуту собираем данные со всех таймслотов в lmq
			int div = int(round((attachments[long_Silense[i]].mes_freq-BEG_FREQ)/BAND_FREQ)); // число от 0 до 14
			lmq.LMQAdd(div+((MemSlot-2)*ABONENTS_MAX));		// от этого пришёл запрос, учитываем слот передачи
															// добавляем в массив в конец для учёта
		  }
		  QQEdit->Text = IntToStr((int)lmq.size());
	  } else {                                              // у ПОДРАЗДЕЛЕНИЙ  - ответы от главного
			String cmes = attachments[long_Silense[0]].attachment;   // для всехподразделений приходит одно управляющее сообщение
			char ans[3];
			char* result = strncpy(ans, (char*)cmes.c_str()+MemSlot*2, 2);
			ans[2] = 0;
			int tto = atoi( ans );
			if (tto>0)	// прислано слоту требователя тишины (всем молчать)
				Silense_Staff = true;
Memo1->Lines->Add("Wait_Obrab="+IntToStr(Silense_local));
			if (division==tto) {			// разрешение лично требователю на посылку MESSAGE_LONG
				Silense_Staff = false;
				Silense_local = false;
				Mode = MESSAGE_LONG;                        // поэтому ставим режим MESSAGE_LONG
			}

Memo1->Lines->Add("Wait_Obrab+="+IntToStr(Silense_local));
	  }
	}

	QueryPerformanceCounter(&EndTime);
	t = (EndTime.QuadPart - CounterStart) / 10000;
//	Edit1->Text = IntToStr(t) + " msec";
}

//-------------------------sound_thread_t---------------------------------
void __fastcall TForm1::ReceiveBtnClick(TObject *Sender) {
	f[0] = '\0';
	if (sound_thread)  {
//		Application->MessageBox ( L"Заканчиваю работу...", L"Сообщение", MB_OK );
		sound_thread = NULL;
		MThreadOff = true;
		INvolume = 100;
		return;
	}
	sound_thread = new sound_thread_t;
}
//---------------------------------------------------------------------

void __fastcall TForm1::ModeNameRGClick(TObject *Sender) {
	if (ModeNameRG->ItemIndex == 0) {
		division = MAIN_KNOT;
		TransmitSlot = 1;
		toEdit->Text = "5";
	} else {
		if (division==MAIN_KNOT) division = 5;
		TransmitSlot = int(division/ABONENTS_MAX) + 2;
		toEdit->Text = "0";
	}
	SNRMenu->Visible = (division==MAIN_KNOT);
	NameEdit->Text = All_Divisions[division];
	NameIndexEdit->Text = IntToStr(division);

	PaintRGreed();
}

 void __fastcall TForm1::NameIndexEditChange(TObject *Sender) {
	int temp;
	temp = NameIndexEdit->Text.ToInt();
	if ((temp>=0) && (temp<=TOTAL_DIVISIONS)) {
		division = temp;
	} else {
		Application->MessageBox ( L"Неверный номер", L"Ошибка", MB_OK );
	}
	ModeNameRGClick(Sender);
 }

void __fastcall TForm1::PaintRGreed(void) {
	if (division==MAIN_KNOT) {
		DivisionNameSG->Visible = true;
		DivisionNameSG->Cells[0][0] = "Подразделение";
		DivisionNameSG->Cells[1][0] = "SNR";
		DivisionNameSG->Cells[2][0] = "Подразделение";
		DivisionNameSG->Cells[3][0] = "SNR";
		DivisionNameSG->Cells[4][0] = "Подразделение";
		DivisionNameSG->Cells[5][0] = "SNR";
		for (int i=1; i < TOTAL_DIVISIONS; i++) {
		  if(i<=ABONENTS_MAX) {
			DivisionNameSG->Cells[0][i] = All_Divisions[i];
			DivisionNameSG->Cells[1][i] = "N/A";
		  } else if (i<=ABONENTS_MAX*2) {
			DivisionNameSG->Cells[2][i-ABONENTS_MAX] = All_Divisions[i];
			DivisionNameSG->Cells[3][i-ABONENTS_MAX] = "N/A";
		  } else {
			DivisionNameSG->Cells[4][i-ABONENTS_MAX*2] = All_Divisions[i];
			DivisionNameSG->Cells[5][i-ABONENTS_MAX*2] = "N/A";
		  }
		}
	} else {
		DivisionNameSG->Visible = false;
	}
	AllColLen(DivisionNameSG);
}

//---------------------------------------------------------------------------

void ColLen(TStringGrid * G,int col) {
		int max = 15;
		for (int row=0; row<G->RowCount; row++) if (max<G->Canvas->TextWidth(G->Cells[col][row])) max=G->Canvas->TextWidth(G->Cells[col][row]);
		G->ColWidths[col]=max+6;
}
void __fastcall TForm1::AllColLen(TStringGrid * G) {
		for (int col = 0; col<G->ColCount; col++) ColLen(G,col);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender) {
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

	Mode = MESSAGE_UNKNOWN;

#ifdef ENG_VARIANT
	ToTranslitBtn->Visible = false;
#else
	ToTranslitBtn->Visible = true;
#endif
	EncryptBtn->Visible = false;
	DecryptBtn->Visible = false;

	WatchTimer->Enabled = false;
	TransLB->TabWidth = 5;
	Silense_local = false;
	Silense_Staff = false;
	Transmiting = false;
	Image1->Visible = false;

	String FIniFileName = LowerCase(Application->ExeName);
	FIniFileName = FIniFileName.SubString(1, FIniFileName.Length() - 3) + "ini";
	IniFile = new TIniFile(FIniFileName);
	Left = IniFile->ReadInteger("FormPos", "Left", 50);
	Top  = IniFile->ReadInteger("FormPos", "Top", 50);
	division  = IniFile->ReadInteger("DivisionPars", "index", 5);

	if (division==MAIN_KNOT) toEdit->Text = "5";

	NameIndexEdit->OnChange = NULL;
	NameIndexEdit->Text = IntToStr(division);
	NameIndexEdit->OnChange = NameIndexEditChange;

	InBar->OnChange = NULL;
	InBar->Position = 20 - (int)(InVol.GetPosition(0, true)*20);
	InBar->OnChange = InBarChange;
	pMeterInfoIN = (IAudioMeterInformation *)InVol.GetInfo();
	INvolume = 100;

	OutBar->OnChange = NULL;
	OutBar->Position = 20 - IniFile->ReadInteger("Faders", "PosOut", 20);
	OutBar->OnChange = OutBarChange;
	OutVol.GetPosition(1, true);
	OutVol.SetPosition(1, 1.0f, true);
	pMeterInfoOUT = (IAudioMeterInformation *)OutVol.GetInfo();
	OUTvolume = 100;

	OutVol.SetMuteOfSysSounds(true);

	SetDevice();

	WatchTimer->Enabled = true;

	if (division>COMMON_CALL) division=MAIN_KNOT;

	if (division==MAIN_KNOT) {
		TransmitSlot = 1;
		ModeNameRG->ItemIndex = 0;
	} else {
		TransmitSlot = int(division/ABONENTS_MAX) + 2;
		ModeNameRG->ItemIndex = 1;
	}
	bmp=ft8_IC->GetBitmap(0,105,105);

	int n = sizeof(rus_mas)/sizeof(char);
	for (int i = 0; i < n; i++)
		tranlit[rus_mas[i]] = eng_mas[i];
	for(std::map <AnsiString, AnsiString>::iterator it = tranlit.begin(); it != tranlit.end(); it++)
		rev_tranlit[(*it).second] = (*it).first;

	NameEdit->Text = All_Divisions[division];
	NameIndexEdit->Text = IntToStr(division);
	ModeNameRG->ItemIndex = division;
	DivisionNameSG->RowCount = ABONENTS_MAX+1;
	SNRMenu->Visible = (division==MAIN_KNOT);

	Path_Messages_Out = ExtractFilePath(Application->ExeName) + "MESSAGES_OUT";
	if (! DirectoryExists("MESSAGES_OUT"))  {
		if (! CreateDir("MESSAGES_OUT")) {
			throw Exception("Не создаётся каталог'+^J+'MESSAGES_OUT");
		}
	}

	Path_Messages_In = ExtractFilePath(Application->ExeName) + "MESSAGES_IN";
	if (! DirectoryExists("MESSAGES_IN"))  {
		if (! CreateDir("MESSAGES_IN")) {
			throw Exception("Не создаётся каталог'+^J+'MESSAGES_IN");
		}
	}

	if (division==MAIN_KNOT) {
		SyncTimeMenu->Checked = IniFile->ReadBool("Modes", "Sync", false);
		if (SyncTimeMenu->Checked)
			SyncTimeMenuClick(Sender);
	}

	PaintRGreed();

	ReceiveBtnClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormDestroy(TObject *Sender) {
	ShellExecute (NULL, NULL, L"taskkill", L"/im player_c.exe /f", NULL, SW_HIDE);
	Transmiting = false;
	Pa_Terminate();

	IniFile->WriteInteger("FormPos", "Left", Left);
	IniFile->WriteInteger("FormPos", "Top", Top);
	IniFile->WriteInteger("DivisionPars", "index", division);
	IniFile->WriteInteger("Faders", "PosOut", 20 - OutBar->Position);
	IniFile->WriteInteger("Modes", "Sync", SyncTimeMenu->Checked);
	IniFile->Free();

	OutVol.SetMuteOfSysSounds(false);
	ReleaseMutex(mutex);
	//И удаляем его.
	CloseHandle(mutex);
}

void __fastcall TForm1::FormCloseQuery(TObject *Sender, bool &CanClose) {
  if (MessageDlg(L"Закрыть приложение?", mtConfirmation, mbOKCancel, 0) == mrCancel)
		CanClose = False;
  else  ThreadsClose();
}

void __fastcall TForm1::CloseActionExecute(TObject *Sender) {
	Close();
}

void __fastcall TForm1::ThreadsClose() {
	if (sound_thread)  {
		sound_thread = NULL;
		time_thread = NULL;
		MThreadOff = true;
		TThreadOff = true;
	}
	Sleep(300);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::DivisionNameSGDrawCell(TObject *Sender, System::LongInt ACol,
		System::LongInt ARow, TRect &Rect, TGridDrawState State) {
	// Если это заголовок
	if (State.Contains(gdFixed)) {
		DivisionNameSG->Canvas->Brush->Color = clHighlight;
		DivisionNameSG->Canvas->Font->Color = clWindowText;
		DivisionNameSG->Canvas->FillRect(Rect);
		Frame3D(DivisionNameSG->Canvas, Rect, clBtnHighlight, clBtnShadow, 1);
	}
	// если ячейка выделена
	else if (State.Contains(gdSelected)) {
		DivisionNameSG->Canvas->Brush->Color = TColor(0x227722);
		DivisionNameSG->Canvas->Font->Color = clHighlightText;
		DivisionNameSG->Canvas->FillRect(Rect);
	} else {
		DivisionNameSG->Canvas->Brush->Color = DivisionNameSG->Color;
		DivisionNameSG->Canvas->Font->Color = DivisionNameSG->Font->Color;
		DivisionNameSG->Canvas->FillRect(Rect);
	}

	AnsiString text = DivisionNameSG->Cells[ACol][ARow];
	if (State.Contains(gdFixed)) {
		DrawTextA(DivisionNameSG->Canvas->Handle, text.c_str(), -1, &Rect,
			 DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
	} else {
		DrawTextA(DivisionNameSG->Canvas->Handle, text.c_str(), -1, &Rect,
			 DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::WatchPanelPaint(TObject *Sender) {
	if (!lineStartPos.IsEmpty() && !lineEndPos.IsEmpty()) {
		WatchPanel->Canvas->Draw(0, 0, bmp);
		WatchPanel->Canvas->MoveTo(lineStartPos.x, lineStartPos.y);
		WatchPanel->Canvas->LineTo(lineEndPos.x, lineEndPos.y);
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::WatchTimerTimer(TObject *Sender) {
	wtime = time(NULL);
	localtime_r(&wtime, &tm_w);
	sec = tm_w.tm_sec;
	min = tm_w.tm_min;
	if(tics++ > 4) {
		tics = 0;
		pMeterInfoOUT->GetPeakValue(&alevelOUT);
		OUTvolume = 100-alevelOUT*100;
		Form1->OutPB->Refresh();
	}
	if (oldsec!=sec) {
		oldsec = sec;
		if (sec<=14)	   CurSlot = 1;
		else if (sec<=29)  CurSlot = 2;
		else if (sec<=44)  CurSlot = 3;
		else 			   CurSlot = 4;
		BYTE masRecip[3] = {0, 0, 0};
		lineStartPos = TPoint();
		lineEndPos = TPoint();
		WatchPanel->Invalidate();
		lineStartPos = TPoint(WatchPanel->Width/2, WatchPanel->Height/2);
		lineEndPos = TPoint(WatchPanel->Width/2 * (1 + sin(sec * 3.1415/30)), WatchPanel->Width/2 * (1 - cos(sec * 3.1415/30)));
		WatchPanel->Invalidate();
		if (sec==59) {                    // в каждое начало минуты   -----------слот 1---------------->
		  Silense_Staff = false;          // у ПОДРАЗДЕЛЕНИЙ снимается запрет на передачу

		  if (division==MAIN_KNOT) {                   // у ГЛАВНОГО - все запросы тишины от подразделений
			int lmqsz = (int)lmq.size();
			if (lmqsz>0) {
LARGE_INTEGER EndTime, StartTime;
int t;
__int64 CounterStart = 0;
QueryPerformanceCounter(&StartTime);
CounterStart = StartTime.QuadPart;

				if ((ddd.size()==0) && NeedPause())  stay_mes_in_buf = true;
Memo1->Lines->Add("NextMess_Obrab="+IntToStr(stay_mes_in_buf));

				int wasSlot = 0;
				bool fare;
				BYTE masRecip[3] = {0, 0, 0};
				for (int i=lmqsz; i > 0; i--) {           // подготовка ближайших требований тишины по таймслотам на текущую минуту
				  fare = false;
				  for (int j=0; j < 3; j++) {              // перебор таймслотов 2-4 (0-2)
					if (((BYTE)(*(masRecip + j))==0) &&    // если в этом таймслоте массив masRecip содержит 0 и
						((lmq.operator [](i-1).member/ABONENTS_MAX)==j)) {    // в lmq есть запрос из этого таймслота
					  *(masRecip + j) = lmq.operator [](i-1).member;     // то кладём туда
					  fare = true;
					  break;
					}
					if (fare)   continue;
				  }
				}

				char senders[14];
				wsprintfA(senders, "%02d%02d%02d", masRecip[0], masRecip[1], masRecip[2]);  // на всю минуту
				MakeMessage(MESSAGE_SILENSE, COMMON_CALL,		// общее сообщение - требование молчания для слотов на 1 минуту
							senders);			                // от которых пришёл вызов

				for (int i=lmqsz; i > 0; i--) {           // удаление из lmq обработанных идентификаторов
				  fare = false;
				  for (int j=0; j < 3; j++) {              // перебор таймслотов 2-4 (0-2)
//					if (((BYTE)(*(masRecip + j))!=0) &&    // если в этом таймслоте массив masRecip не содержит 0 и
					if (lmq.operator [](i-1).member==(BYTE)(*(masRecip + j))) {    // в lmq есть запрос из этого таймслота
					  lmq.erase(i-1);     // то удаляем
					  fare = true;
					  break;
					}
					if (fare)   continue;
				  }
				}
				QQEdit->Text = IntToStr((int)lmq.size());
QueryPerformanceCounter(&EndTime);
t = (EndTime.QuadPart - CounterStart) / 10000;
Edit1->Text = IntToStr(t) + " msec";
			}
		  }

		  if ((!(Silense_local || Silense_Staff)              // если НЕ установлено ожидание или режим молчания
			 || (Mode==MESSAGE_SILENSE)) && (ddd.size()>0))
						NextFromBufBtnClick(Sender);            // стирание переданного сообщения и переход к новому
		  if ((division==MAIN_KNOT) && ModeSNR) {
			char mas[14];
			sprintf(mas, "%s", "SNRREQUEST");
			MakeMessage(MESSAGE_REQ_SNR, COMMON_CALL, mas);
		  }
		}

		if ((sec==0) || (sec==15) || (sec==30) || (sec==45))
			if (Transmiting)  	Transmiting = false;

		int t1 = (division==MAIN_KNOT) ? 0 : (division/ABONENTS_MAX+1)*15;
		if (sec==t1) {
			LongMessTransmitter = (CurSlot>1) ? masRecip[CurSlot-2] : MAIN_KNOT;   // либо MAIN_KNOT, либо код подразделения в текущем таймслоте
			if (!(Silense_local || Silense_Staff))	PlayNewBtnClick(this);
		}
	}
}

//---------------------------------------------------------------------------

void __fastcall TForm1::PlayOut(void) {
	String ParamString;
									  // 6 - 'это команда для выдачи информации для звука'
	ParamString = Format("%d%d%2d%2d%s", 6, ddd.operator [](0).type, ddd.operator [](0).to, division, ddd.operator [](0).mess);
	Transmiting = true;

	try {
			TCPClient->Host = "127.0.0.1";
			if(!TCPClient->Connected() ) TCPClient->Connect();

			TCPClient->Socket->WriteLn(ParamString, enUTF8);
			Memo1->Lines->Add(TCPClient->Socket->ReadLn(enUTF8));
	}
	__finally {
			 TCPClient->Disconnect();
	}

	if ((Mode==MESSAGE_SILENSE) && (division!=MAIN_KNOT)) { // у ПОДРАЗДЕЛЕНИЙ
		Silense_local = true;
		Memo1->Lines->Add("Wait_PlayOut="+IntToStr(Silense_local));
	}
}

void __fastcall TForm1::SetDevice(void) {
	String ParamString;
									  // 9 - 'это команда для выбора звукового устройства'
	ParamString = Format("%d%d", 9, RealDevice(1));

	try {
			TCPClient->Host = "127.0.0.1";
			if(!TCPClient->Connected() ) TCPClient->Connect();

			TCPClient->Socket->WriteLn(ParamString, enUTF8);
			Memo1->Lines->Add(TCPClient->Socket->ReadLn(enUTF8));
	}
	__finally {
			 TCPClient->Disconnect();
	}
}

//---------------------------------------------------------------------------

void __fastcall TForm1::PlayNewBtnClick(TObject *Sender){
	if (ddd.size()>0)	PlayOut();
}

//---------------------------------------------------------------------------

void __fastcall TForm1::TextBtnClick(TObject *Sender) {
#ifdef ENG_VARIANT
	mesEdit->Text = "The schematic is the same as in the original article, but includes specific component values, construction details, and alignment procedures. T1 consists of 35 turns of fine enameled wire wound on a T68-2 toroid, with a length of coaxial cable passed through the center of the core.";
#else
	mesEdit->Text = "Схема та!@#$ %^& же, что в ор|и/гин^а()льн{}ой ста`тье, но с указанием конкретных номиналов компонентов, способа изготовления устройства и его настройки. T1 представляет собой 35 витков тонкого эмалированного провода на кольце T68-2. В это кольцо продевается отрезок коаксиального кабеля.";
#endif
	String s = mesEdit->Text;
	char buff[500];
	wsprintfA(buff, "%ls", s.c_str());
	buff[499] = '\0';
	s=regex_replace(buff, std::regex("[`'\"!<>@#%_+()\\^$&/{}*|]"), "");
	mesEdit->Text = s;
}

AnsiString __fastcall TForm1::Translit(AnsiString Source) {
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

AnsiString __fastcall TForm1::rev_Translit(AnsiString Source) {
	AnsiString Result;
	AnsiString Str = Source + " ";
	AnsiString Temp;

	for(int i = 0; i < Source.Length(); i++) {
		Temp = rev_tranlit[AnsiString(Str[i + 1]) + AnsiString(Str[i + 2])];
		if(Temp != "") {
		  Result += Temp;
		  i++;
		} else {
			AnsiString e = rev_tranlit[AnsiString(Str[i + 1])];
			if (e.Length() > 0) {
				Result += rev_tranlit[AnsiString(Str[i + 1])];
			} else {
				Result += Str[i + 1];
			}
		}
	}
	return Result.UpperCase();
}

void __fastcall TForm1::ToTranslitBtnClick(TObject *Sender) {
	String s = Translit(mesEdit->Text.LowerCase());
    mesEdit->Text = s;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InBarChange(TObject *Sender) {
	InVol.SetPosition(0, (double)(20 - InBar->Position)/20, true);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::OutBarChange(TObject *Sender) {
	OutVol.SetPositionOfSoundSessions((float)(20 - OutBar->Position)/20);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InPBPaint(TObject *Sender) {
	RECT rect = InPB->ClientRect;
	rect.top += INvolume;
	HBRUSH brush = CreateSolidBrush(RGB(0, 200, 255));
	FillRect(InPB->Canvas->Handle, &rect, brush);
	DeleteObject(brush);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OutPBPaint(TObject *Sender) {
	RECT rect = OutPB->ClientRect;
	rect.top += OUTvolume;
	HBRUSH brush = CreateSolidBrush(RGB(138, 222, 155));
	FillRect(OutPB->Canvas->Handle, &rect, brush);
	DeleteObject(brush);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormPaint(TObject *Sender) {
	RECT rect = InPB->ClientRect;
	rect.top += InPB->Top-2;
	rect.left += InPB->Left-2;
	rect.right += (InPB->Left + 2);
	rect.bottom += (InPB->Top + 2);
	HBRUSH brush = CreateSolidBrush(RGB(222, 222, 222));
	FillRect(Form1->Canvas->Handle, &rect, brush);
	DeleteObject(brush);

	rect = OutPB->ClientRect;
	rect.top += OutPB->Top-2;
	rect.left += OutPB->Left-2;
	rect.right += (OutPB->Left + 2);
	rect.bottom += (OutPB->Top + 2);
	brush = CreateSolidBrush(RGB(222, 222, 222));
	FillRect(Form1->Canvas->Handle, &rect, brush);
	DeleteObject(brush);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::EncryptBtnClick(TObject *Sender) {
//	mesEdit->Text = Enigma.Encrypt(mesEdit->Text);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::DecryptBtnClick(TObject *Sender) {
//	mesEdit->Text = Enigma.Decrypt(mesEdit->Text);
}

//---------------------------------------------------------------------------

void __fastcall TForm1::SyncTimeMenuClick(TObject *Sender) {

	if (!SyncTimeMenu->Checked) {
		SNRMenu->Enabled = false;      // нельзя оновременно

		Mode = MESSAGE_TIME;
		Image1->Picture->Bitmap->Assign(ft8_IC->GetBitmap(2,105,56));
		Image1->Visible = true;

		if (sound_thread)  {
			MThreadOff = true;
			Sleep(200);
			sound_thread = NULL;
			INvolume = 100;
			Form1->InPB->Refresh();
		}

		time_thread = new time_thread_t;
		TThreadOff = false;
		SyncTimeMenu->Checked = true;

	} else {
		Mode = MESSAGE_UNKNOWN;
        Image1->Visible = false;

		if (time_thread)  {
			TThreadOff = true;
			Sleep(200);
			SNRMenu->Enabled = true;
			time_thread = NULL;
			INvolume = 100;
			Form1->InPB->Refresh();
		}

		sound_thread = new sound_thread_t;
		MThreadOff = false;
		SyncTimeMenu->Checked = false;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::SNRMenuClick(TObject *Sender) {      // request SNR by major

	if (!SNRMenu->Checked) {
		SyncTimeMenu->Enabled = false;      // нельзя оновременно
		Mode = MESSAGE_REQ_SNR;
		ModeSNR = true;
		Image1->Visible = true;
		Image1->Picture->Bitmap->Assign(ft8_IC->GetBitmap(1, 105, 56));

		Memo1->Lines->Add("--общий запрос (63) всем подразделениям--");
		Memo1->Lines->Add("---------приём от подразделений----------");
		SNRMenu->Checked = true;
	} else {
		Mode = MESSAGE_UNKNOWN;
		ModeSNR = false;
		Image1->Visible = false;
		SNRMenu->Checked = false;
		SyncTimeMenu->Enabled = true;
	}
	IniFile->WriteBool("Modes", "SNR", SNRMenu->Checked);
}

//---------------------------------------------------------------------------
bool __fastcall TForm1::NeedPause ( void ) {
	switch (TransmitSlot) {
		case 1:
			return ((sec>=00) && (sec<=59));
		case 2:
			return ((sec>=15) && (sec<=60));
		case 3:
			return ((sec>=30) && (sec<=60));
		case 4:
			return ((sec>=45) && (sec<=60));
	}
	return (false);
}

void __fastcall TForm1::MessToBufferBtnClick(TObject *Sender) {
	String s = Trim(mesEdit->Text);
	if (s.Length()>0) {
		if ((ddd.size()==0) && NeedPause())  stay_mes_in_buf = true;
Memo1->Lines->Add("NextMess_ToBuffer="+IntToStr(stay_mes_in_buf));

		char buff[400];
		wsprintfA(buff, "%.500ls", s.c_str());
		buff[399] = '\0';
		std::string xStr = buff;
		xStr=regex_replace(xStr, std::regex("[-]"), " ");
		xStr=regex_replace(xStr, std::regex("[,]"), ".");
		s=regex_replace(xStr, std::regex("[`'\"!<>@#%_+()\\^$&/{}*|]"), "");

#ifdef ENG_VARIANT
		s = AnsiString(s).LowerCase();
#else
		s = Translit(AnsiString(s).LowerCase());
#endif
		if (s.Length()>24) 	Mode = MESSAGE_LONG;
		else 	Mode = MESSAGE_STANDART;

//		wsprintfA(buff, "%.400ls", Enigma.Encrypt(s).c_str());
		wsprintfA(buff, "%.400ls", s.c_str());

		MakeMessage(Mode, toEdit->Text.ToInt(), buff);

	} else
		Application->MessageBox (L"Пустое сообщение", L"Сообщение", MB_OK );
}
//---------------------------------------------------------------------------

void __fastcall TForm1::PopUpMenuShowMessClick(TObject *Sender) {
	if (TransLB->ItemIndex == -1) {
		TransLB->ItemIndex = 0;
	}
	float snr=0, ans_freq=0;
	int i3=0;
	char buff[22];
	String s = decode_ft8(ddd.operator[](TransLB->ItemIndex).mess, &i3, &snr, &ans_freq);
	sprintf(buff, " (%.1f dB  %.1f Hz)", 33.1 - snr, ans_freq);
	Memo1->Lines->Add (s + buff);
	Application->MessageBox ((TransLB->Items->Strings[TransLB->ItemIndex]+ "\n"+s).c_str(), L"Сообщение", MB_OK );
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TransLBDblClick(TObject *Sender) {
	PopUpMenuShowMessClick(Sender);
}

void __fastcall TForm1::ClearBtnClick(TObject *Sender) {
	Memo1->Clear();
}
//---------------------------------------------------------------------------


void __fastcall TForm1::OutputsCBChange(TObject *Sender) {
   SetDevice();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button1Click(TObject *Sender)
{
/*		bool fare;
		lmq.LMQAdd(11);		// от этого пришёл запрос, учитываем слот передачи
		lmq.LMQAdd(5);		// от этого пришёл запрос, учитываем слот передачи
		lmq.LMQAdd(41);		// от этого пришёл запрос, учитываем слот передачи
	  int lmqsz = (int)lmq.size();

			int wasSlot = 0;
			BYTE masRecip[3] = {0, 0, 0};
			for (int i=lmqsz; i > 0; i--) {           // подготовка ближайших требований тишины по таймслотам на текущую минуту
			  fare = false;
			  for (int j=0; j < 3; j++) {              // перебор таймслотов 2-4 (0-2)
				if (((BYTE)(*(masRecip + j))==0) &&    // если в этом таймслоте массив masRecip содержит 0 и
					((lmq.operator [](i-1).member/ABONENTS_MAX)==j)) {    // в lmq есть запрос из этого таймслота
				  *(masRecip + j) = lmq.operator [](i-1).member;     // то кладём туда
				  fare = true;
				  break;
				}
				if (fare)   continue;
			  }
			}

			char senders[14];
			wsprintfA(senders, "%02d%02d%02d", masRecip[0], masRecip[1], masRecip[2]);

			for (int i=lmqsz; i > 0; i--) {           // удаление из lmq обработанных идентификаторов
			  fare = false;
			  for (int j=0; j < 3; j++) {              // перебор таймслотов 2-4 (0-2)
//				if (((BYTE)(*(masRecip + j))!=0) &&    // если в этом таймслоте массив masRecip не содержит 0 и
				if (lmq.operator [](i-1).member==(BYTE)(*(masRecip + j))) {  // в lmq есть запрос из этого таймслота
int t = lmq.operator [](i-1).member;
				  lmq.erase(i-1);     // то удаляем
				  fare = true;
				  break;
				}
				if (fare)   continue;
			  }
			}
	   lmqsz = (int)lmq.size();
			wsprintfA(senders, "%02d%02d%02d", masRecip[0], masRecip[1], masRecip[2]);  // на всю минуту
			MakeMessage(MESSAGE_SILENSE, COMMON_CALL,		// общее сообщение - требование молчания для слотов на 1 минуту
				senders);			                // от которых пришёл вызов
*/
			char ll[ATTACHMENTS_MAX][14] = {'J','C','9','W','V','S','I','F','5','5','A','F','\0','\0','U','1','J','B','G','Z','V','\0'};
			char lll[400];
            lll[0] = '\0';
			char * p = lll;
			for ( int i=0; i<2; i++)
				p = append_string(p, ll[i]);

			String qqq = "JC9WVSIF55AFU1JBGZV";
//			String aaa = Enigma.Decrypt(qqq);
			String aaa = qqq;

//			Enigma.Decrypt_ptr(lll);

			char text[22] = {'\0'};
			sprintf(text, " (%.1f dB, частей %d)", 33.1 - 5, 2);
			p = append_string(p, text);

}
//---------------------------------------------------------------------------

