#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <Windows.h>

#include "include/portaudio.h"
#include "common/common.h"
#include "common/audio.h"
#include "ft8/message.h"
#include "ft8/text.h"

//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Server_u.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
TCriticalSection *section;
int processed;
char mess[400];
char wav_out[256];
PaError pa_rc;

float BAND_FREQ = (float)BAND_USED/ABONENTS_MAX;
int division;
float slot_period = 15;
int sample_rate = 12000, num_samples, dev=4;;
float* signal;

int FindFreqForDivision(int mode) {
	if (division==MAIN_KNOT)
		return BEG_FREQ;
	else if(mode==MESSAGE_LONG)
		return (BEG_FREQ);
	else
		return (BEG_FREQ + BAND_FREQ * (division - int(division/ABONENTS_MAX)*ABONENTS_MAX));
}
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner): TForm(Owner)  {
	char original[200];

	pa_rc = Pa_Initialize(); // Initialize PortAudio
	if (pa_rc != paNoError) {
		txtResults->Lines->Add("Error initializing PortAudio.");
		sprintf(original,"Errortext: %s Number: %d", Pa_GetErrorText(pa_rc), pa_rc);
		txtResults->Lines->Add(original);
		Pa_Terminate(); // I don't think we need this but...
	} else
		txtResults->Lines->Add("PortAudio initialized.");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::IdTCPServer1Execute(TIdContext *AContext) {

LARGE_INTEGER EndTime, StartTime;

int t;
__int64 CounterStart = 0;
QueryPerformanceCounter(&StartTime);
CounterStart = StartTime.QuadPart;

  String strText = AContext->Connection->Socket->ReadLn(enUTF8);
  int vid  = (strText.SubString(1, 1)).ToInt();
  if (vid==6) {
	int type = (strText.SubString(2, 1)).ToInt();
	int to   = (strText.SubString(3, 2)).ToInt();
	division = (strText.SubString(5, 2)).ToInt();

	wsprintfA(mess, "%ls", strText.SubString(7, 400).c_str());

	if (strlen(mess)>0) {
		AContext->Connection->Socket->WriteLn("AudioServer OK", enUTF8);

		memset(signal, 0, num_samples*sizeof(float));
		gen_ft8_hor( to, type, mess, signal);

		section->Enter();
		processed += 1;
		txtCount->Text = IntToStr(processed);
		txtFrom->Text = AContext->Connection->Socket->Binding->PeerIP;
		section->Leave();

		AContext->Connection->Disconnect();

		num_samples = (slot_period - 0.4f) * sample_rate;

		sprintf(wav_out, "Sample rate %d Hz, %d samples, %.3f seconds", sample_rate, num_samples, (double)num_samples / sample_rate);
		txtResults->Lines->Add(wav_out);

		for (int frame_pos = 0; frame_pos + 2048 <= num_samples; frame_pos += 2048) {
			audio_write(&signal[frame_pos], 2048);
			if (frame_pos==0) {
QueryPerformanceCounter(&EndTime);
t = (EndTime.QuadPart - CounterStart) / 10000;
PerformEd->Text = IntToStr(t) + " msec";
			}
		}
		float short_tail[2] = {0.0f, 0.0f}; // Весит ровно 8 байт (2 сэмпла)
		audio_write(short_tail, 2);
	}
  } else if (vid==9) {
		AContext->Connection->Socket->WriteLn("AudioServer OK", enUTF8);

		dev = (strText.SubString(2, 1)).ToInt();
//		Pa_Terminate();
		audio_open(dev, 1);
		sprintf(wav_out, "Audio Device - %d", dev);
		txtResults->Lines->Add(wav_out);

		section->Enter();
		processed += 1;
		txtCount->Text = IntToStr(processed);
		txtFrom->Text = AContext->Connection->Socket->Binding->PeerIP;
		section->Leave();

		AContext->Connection->Disconnect();
  }
}

//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender) {
	if (pa_rc == paNoError) {
		section = new TCriticalSection;
		num_samples = slot_period * sample_rate;
		signal = (float*)malloc(num_samples*sizeof(float));
		audio_init();
		audio_open(dev, 1);
		sprintf(wav_out, "Audio Device - %d", dev);
		txtResults->Lines->Add(wav_out);
	}
}

//---------------------------------------------------------------------------


void __fastcall TForm1::FormDestroy(TObject *Sender) {
	free(signal);
	section->Free();
	Pa_Terminate();
}
//---------------------------------------------------------------------------
