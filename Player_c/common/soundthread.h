#include "FT8transport.h"
#include "vol.h"

const int kFreq_osr = 2; // Frequency oversampling rate (bin subdivision)
const int kTime_osr = 2; // Time oversampling rate (symbol subdivision)
extern char letter[400];
extern bool Transmiting;
extern int TransmitSlot, INvolume, MemSlot, CurSlot;
extern IAudioMeterInformation *pMeterInfoIN;
extern float alevelIN;
extern monitor_t mon_Main;

//-------------------------sound_thread_t---------------------------------
class sound_thread_t;
static sound_thread_t *sound_thread;

bool MThreadOff = false;

class sound_thread_t : public TThread {
public:
  sound_thread_t(bool CreateSuspended = false): TThread(CreateSuspended) {
		FreeOnTerminate = true;
  }

  void __fastcall beg(void) {
		Form1->ProgressBar1->Position = 0;
  }

  void __fastcall ris(void) {
		Form1->InPB->Refresh();
		Form1->ProgressBar1->Position += 1;
 }

  void __fastcall obrabotka(void) {
		MemSlot = CurSlot;
		Form1->Obrab_Thread();
 }

  void __fastcall end(void) {
		INvolume = 100;
		Form1->InPB->Refresh();
		Form1->ProgressBar1->Position = 100;
		sound_thread = NULL;
		Pa_Terminate();
  }

  virtual void __fastcall Execute(void) {
	Synchronize(beg);

	ftx_protocol_t protocol = FTX_PROTOCOL_FT8;
	float time_shift = 0.8, slot_period;
	struct tm tm_struct = { 0 };
	time_t mytime;
	int sec, oldslot, slot;

	int sample_rate = 12000, num_samples;
	monitor_t mon;
	monitor_config_t mon_cfg;

	slot_period = ((protocol == FTX_PROTOCOL_FT8) ? FT8_SLOT_TIME : FT4_SLOT_TIME);
	num_samples = slot_period * sample_rate;
	float * signal = (float*)malloc(num_samples*sizeof(float));

	audio_init();
	if(audio_open(Form1->RealDevice(0), 0) != 0) MThreadOff = true;
	num_samples = (slot_period - 0.4f) * sample_rate;

	// Compute FFT over the whole signal and store it
	mon_cfg.f_min = 200;
	mon_cfg.f_max = 3000;
	mon_cfg.sample_rate = sample_rate;
	mon_cfg.time_osr = kTime_osr;
	mon_cfg.freq_osr = kFreq_osr;
	mon_cfg.protocol = protocol;

	hashtable_init();

	monitor_init(&mon, &mon_cfg);

	do {

		// Wait for the start of time slot
		mytime = time(NULL);
		localtime_r(&mytime, &tm_struct);
		sec = tm_struct.tm_sec;
		oldslot = sec/15 + 1;
		while (true) {
			mytime = time(NULL);
			localtime_r(&mytime, &tm_struct);
			sec = tm_struct.tm_sec;
			slot = sec/15 + 1;

			Sleep(100);

			if ((oldslot!=slot) && !Transmiting) break;   // do not listen in transmit time
//			if (oldslot!=slot) break;
			oldslot = slot;

			if (MThreadOff) break;
		}

		Synchronize(beg);

		// Process and accumulate audio data in a monitor/waterfall instance
		for (int frame_pos = 0; frame_pos + mon.block_size <= num_samples; frame_pos += mon.block_size) {

			if (MThreadOff || Transmiting) break;   // do not listen in transmit time

			audio_read(signal + frame_pos, mon.block_size);

			// Process the waveform data frame by frame - you could have a live loop here with data from an audio device
			monitor_process(&mon, signal + frame_pos);

			pMeterInfoIN->GetPeakValue(&alevelIN);
			INvolume = 100-alevelIN*100;
			Synchronize(ris);
		}

		mon_Main = mon;

		Synchronize(obrabotka);

		monitor_reset(&mon);

		if (MThreadOff) {
		  sound_thread = NULL;
		  MThreadOff = false;
		  break;
		}
	} while (! MThreadOff);

	monitor_free(&mon);
	free(signal);

	Synchronize(end);
  }
};

//---------------------------------------------------------------------
