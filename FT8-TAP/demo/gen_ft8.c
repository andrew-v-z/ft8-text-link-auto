#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "common/common.h"
#include "common/wave.h"
#include "ft8/message.h"
#include "ft8/encode.h"
#include "ft8/constants.h"
#include <ft8/text.h>

//#define LOG_LEVEL LOG_INFO
//#include "ft8/debug.h"

#define FT8_SYMBOL_BT 2.0f ///< symbol smoothing filter bandwidth factor (BT)
#define FT4_SYMBOL_BT 1.0f ///< symbol smoothing filter bandwidth factor (BT)

#define GFSK_CONST_K 5.336446f ///< == pi * sqrt(2 / log(2))

/// Computes a GFSK smoothing pulse.
/// The pulse is theoretically infinitely long, however, here it's truncated at 3 times the symbol length.
/// This means the pulse array has to have space for 3*n_spsym elements.
/// @param[in] n_spsym Number of samples per symbol
/// @param[in] b Shape parameter (values defined for FT8/FT4)
/// @param[out] pulse Output array of pulse samples
///
void gfsk_pulse(int n_spsym, float symbol_bt, float* pulse) {
	int i;
	for ( i = 0; i < 3 * n_spsym; ++i) {
		float t = i / (float)n_spsym - 1.5f;
        float arg1 = GFSK_CONST_K * symbol_bt * (t + 0.5f);
        float arg2 = GFSK_CONST_K * symbol_bt * (t - 0.5f);
		pulse[i] = (erff(arg1) - erff(arg2)) / 2;
    }
}

/// Synthesize waveform data using GFSK phase shaping.
/// The output waveform will contain n_sym symbols.
/// @param[in] symbols Array of symbols (tones) (0-7 for FT8)
/// @param[in] n_sym Number of symbols in the symbol array
/// @param[in] f0 Audio frequency in Hertz for the symbol 0 (base frequency)
/// @param[in] symbol_bt Symbol smoothing filter bandwidth (2 for FT8, 1 for FT4)
/// @param[in] symbol_period Symbol period (duration), seconds
/// @param[in] signal_rate Sample rate of synthesized signal, Hertz
/// @param[out] signal Output array of signal waveform samples (should have space for n_sym*n_spsym samples)
///
void synth_gfsk(const uint8_t* symbols, int n_sym, float f0, float symbol_bt, float symbol_period, int signal_rate, float* signal) {
	int n_spsym = (int)(0.5f + signal_rate * symbol_period); // Samples per symbol
	int n_wave = n_sym * n_spsym, i,j,k,n_ramp;              // Number of output samples
	float hmod = 1.0f, dphi_peak;
	float* dphi, *pulse;
	float phi = 0;

//    LOG(LOG_DEBUG, "n_spsym = %d\n", n_spsym);
	// Compute the smoothed frequency waveform.
	// Length = (nsym+2)*n_spsym samples, first and last symbols extended
	dphi_peak = 2 * M_PI * hmod / n_spsym;
	dphi = (float*)malloc((n_wave + 2 * n_spsym)*sizeof(float));
//	float dphi[n_wave + 2 * n_spsym];

	// Shift frequency up by f0
	for ( i = 0; i < n_wave + 2 * n_spsym; ++i) {
		dphi[i] = 2 * M_PI * f0 / signal_rate;
	}

	pulse = (float*)malloc((3 * n_spsym)*sizeof(float));
//	float pulse[3 * n_spsym];
	gfsk_pulse(n_spsym, symbol_bt, pulse);

	for ( i = 0; i < n_sym; ++i) {
		int ib = i * n_spsym;
		for ( j = 0; j < 3 * n_spsym; ++j)  {
			dphi[j + ib] += dphi_peak * symbols[i] * pulse[j];
		}
	}

	// Add dummy symbols at beginning and end with tone values equal to 1st and last symbol, respectively
	for ( j = 0; j < 2 * n_spsym; ++j) {
		dphi[j] += dphi_peak * pulse[j + n_spsym] * symbols[0];
		dphi[j + n_sym * n_spsym] += dphi_peak * pulse[j] * symbols[n_sym - 1];
	}

	// Calculate and insert the audio waveform
	phi = 0;
	for ( k = 0; k < n_wave; ++k) { // Don't include dummy symbols
		signal[k] = sinf(phi);
		phi = fmodf(phi + dphi[k + n_spsym], 2 * M_PI);
	}

	// Apply envelope shaping to the first and last symbols
	n_ramp = n_spsym / 8;
	for ( i = 0; i < n_ramp; ++i) {
		float env = (1 - cosf( M_PI * i / n_ramp )) / 2;
		signal[i] *= env;
		signal[n_wave - 1 - i] *= env;
	}
	free(dphi);
	free(pulse);
}

void usage1(){
	printf("Generate a 15-second WAV file encoding a given message.\n");
	printf("Usage:\n");
	printf("\n");
	printf("gen_ft8 MESSAGE WAV_FILE [FREQUENCY]\n");
	printf("or\n");
	printf("gen_ft8 -m MESSAGE\n");
	printf("(Note that you might have to enclose your message in quote marks if it contains spaces)\n");
}

void gen_ft8( int adresat, int mode, int num_mes, const char * message, char * wav_path) {
	int l, i, j, num_tones;
	int sample_rate = 12000, num_samples, num_silence, num_total_samples;
	float frequency = 1000.0, symbol_period, symbol_bt, slot_time;
	bool is_ft4=false;
	char temp[29];
	ftx_message_t msg;
	uint8_t *tones;
	float *signal;

	frequency = (float)FindFreqForDivision(mode) + TF8_BAND_FREQ;

//	is_ft4 = (argc > 4) && (0 == strcmp(argv[4], "-ft4"));

	ftx_message_rc_t rc;
	rc = ftx_message_encode(&msg, message);

	if (rc != FTX_MESSAGE_RC_OK) {
		printf("RC = %d\n", (int)rc);
	}

	num_tones = (is_ft4) ? FT4_NN : FT8_NN;
	symbol_period = (is_ft4) ? FT4_SYMBOL_PERIOD : FT8_SYMBOL_PERIOD;
	symbol_bt = (is_ft4) ? FT4_SYMBOL_BT : FT8_SYMBOL_BT;
	slot_time = (is_ft4) ? FT4_SLOT_TIME : FT8_SLOT_TIME;

	// Second, encode the binary message as a sequence of FSK tones
	tones = (uint8_t*)malloc(num_tones*sizeof(uint8_t));
	//	uint8_t tones[num_tones]; // Array of 79 tones (symbols)
	if (is_ft4) {
		ft4_encode(msg.payload, tones);
	} else {
		ft8_encode(msg.payload, tones);
	}

	// Third, convert the FSK tones into an audio signal
	num_samples = (int)(0.5f + num_tones * symbol_period * sample_rate); // Number of samples in the data signal
	num_silence = (slot_time * sample_rate - num_samples) / 2;           // Silence padding at both ends to make 15 seconds
	num_total_samples = num_silence + num_samples + num_silence;         // Number of samples in the padded signal
	signal = (float*)malloc(num_total_samples*sizeof(float));
	//	float signal[num_total_samples];
	for ( i = 0; i < num_silence; i++) {
		signal[i] = 0;
		signal[i + num_samples + num_silence] = 0;
	}

	// Synthesize waveform data (signal) and save it as WAV file
	synth_gfsk(tones, num_tones, frequency, symbol_bt, symbol_period, sample_rate, signal + num_silence);
	char * p;
	temp[0] = '\0';
	p = (char *)wav_path;
	sprintf(temp, "\\adr%02dmod%dc%02d.wav", adresat, mode, num_mes);
	p = append_string(p, temp);
	save_wav(signal, num_total_samples, sample_rate, wav_path);

	free(tones);
	free(signal);
}

void gen_ft8_hor( int adresat, int mode, const char * message, float * wav_mass) {
	int l, i, num_tones, count;
	int sample_rate = 12000, num_samples, num_silence, num_total_samples;
	float frequency = 1000.0, symbol_period, symbol_bt, slot_time;
	bool is_ft4=false;
	char mout[256];
	char temp[29];
	ftx_message_t msg;
	uint8_t *tones[MAX_MESS_QNT];
	float *signal[MAX_MESS_QNT];
//	float *signal_sum;
	ftx_message_rc_t rc;

	num_tones = (is_ft4) ? FT4_NN : FT8_NN;
	symbol_period = (is_ft4) ? FT4_SYMBOL_PERIOD : FT8_SYMBOL_PERIOD;
	symbol_bt = (is_ft4) ? FT4_SYMBOL_BT : FT8_SYMBOL_BT;
	slot_time = (is_ft4) ? FT4_SLOT_TIME : FT8_SLOT_TIME;

	// Third, convert the FSK tones into an audio signal
	num_samples = (int)(0.5f + num_tones * symbol_period * sample_rate); // Number of samples in the data signal
	num_silence = (slot_time * sample_rate - num_samples) / 2;           // Silence padding at both ends to make 15 seconds
	num_total_samples = num_silence + num_samples + num_silence;         // Number of samples in the padded signal

//	signal_sum = (float*)malloc(num_total_samples*sizeof(float));

	l = strlen(message);
	for (count = 0; count < l; count++) {
		char tempo[13] = { 0 };
		if (l >= (count * 12 + 12))
			strncpy(tempo, message + count * 12, 12);
		else {
			char* p;
			strncpy(tempo, message + count * 12, l - count * 12);
			p = tempo;
			while (strlen(tempo) < 12)
				p = append_string(p, " ");
		}

		sprintf(temp, "%02d %02d %01d %s", adresat, count, mode, tempo);

		rc = ftx_message_encode(&msg, temp);

		// Second, encode the binary message as a sequence of FSK tones
		tones[count] = (uint8_t*)malloc(num_tones*sizeof(uint8_t));
		// Array of 79 tones (symbols)
		if (is_ft4) {
			ft4_encode(msg.payload, tones[count]);
		} else {
			ft8_encode(msg.payload, tones[count]);
		}

		signal[count] = (float*)malloc(num_total_samples*sizeof(float));

		for ( i = 0; i < num_silence; i++) {
			signal[count][i] = 0;
			signal[count][i + num_samples + num_silence] = 0;
		}

		frequency = (float)FindFreqForDivision(mode) + TF8_BAND_FREQ * count;

		// Synthesize waveform data (signal) and save it as WAV file
		synth_gfsk(tones[count], num_tones, frequency, symbol_bt, symbol_period, sample_rate, signal[count] + num_silence);

		for (i = 0; i < num_total_samples; i++)
//			signal_sum[i] += signal[count][i];
			wav_mass[i] += signal[count][i];
		if (l <= (count * 12 + 12))
			break;
	}

	count++;
	float norm = sqrtf((float)count) * 1.15f;
	for (i = 0; i < num_total_samples; i++)
//		signal_sum[i] /= count;

//	wav_mass[i] /= count;
	wav_mass[i] = tanhf(wav_mass[i] / norm);  // гиперболический тангенс вместо обрезки

	for ( i = 0; i < count; i++)
		free(tones[i]);
	for ( i = 0; i < count; i++)
		free(signal[i]);

//	wav_mass = signal_sum;
}

void gen_ft8_hor_to_file( int adresat, int mode, const char * message, char * wav_path) {
	int l, i, num_tones, count = 0;
	int sample_rate = 12000, num_samples, num_silence, num_total_samples;
	float frequency = 1000.0, symbol_period, symbol_bt, slot_time;
	bool is_ft4=false;
	char mout[256];
	char temp[29];
	ftx_message_t msg;
	uint8_t *tones[MAX_MESS_QNT];
	float *signal[MAX_MESS_QNT];
	float *signal_sum;
	ftx_message_rc_t rc;

	num_tones = (is_ft4) ? FT4_NN : FT8_NN;
	symbol_period = (is_ft4) ? FT4_SYMBOL_PERIOD : FT8_SYMBOL_PERIOD;
	symbol_bt = (is_ft4) ? FT4_SYMBOL_BT : FT8_SYMBOL_BT;
	slot_time = (is_ft4) ? FT4_SLOT_TIME : FT8_SLOT_TIME;

	// Third, convert the FSK tones into an audio signal
	num_samples = (int)(0.5f + num_tones * symbol_period * sample_rate); // Number of samples in the data signal
	num_silence = (slot_time * sample_rate - num_samples) / 2;           // Silence padding at both ends to make 15 seconds
	num_total_samples = num_silence + num_samples + num_silence;         // Number of samples in the padded signal

	signal_sum = (float*)malloc(num_total_samples*sizeof(float));

	l = strlen(message);
	for (count = 0; count < l; count++) {
		char tempo[13] = { 0 };
		if (l >= (count * 12 + 12))
			strncpy(tempo, message + count * 12, 12);
		else {
			char* p;
			strncpy(tempo, message + count * 12, l - count * 12);
			p = tempo;
			while (strlen(tempo) < 12)
				p = append_string(p, " ");
		}

		sprintf(temp, "%02d %02d %01d %s", adresat, count, mode, tempo);

		rc = ftx_message_encode(&msg, temp);

		// Second, encode the binary message as a sequence of FSK tones
		tones[count] = (uint8_t*)malloc(num_tones*sizeof(uint8_t));
		// Array of 79 tones (symbols)
		if (is_ft4) {
			ft4_encode(msg.payload, tones[count]);
		} else {
			ft8_encode(msg.payload, tones[count]);
		}

		signal[count] = (float*)malloc(num_total_samples*sizeof(float));

		for ( i = 0; i < num_silence; i++) {
			signal[count][i] = 0;
			signal[count][i + num_samples + num_silence] = 0;
		}

		frequency = (float)FindFreqForDivision(mode) + TF8_BAND_FREQ * count;

		// Synthesize waveform data (signal) and save it as WAV file
		synth_gfsk(tones[count], num_tones, frequency, symbol_bt, symbol_period, sample_rate, signal[count] + num_silence);

		for (i = 0; i < num_total_samples; i++)
			signal_sum[i] += signal[count][i];

		if (l <= (count * 12 + 12))
			break;
	}

	count++;
	for (i = 0; i < num_total_samples; i++)
		signal_sum[i] /= count;

	for ( i = 0; i < count; i++)
		free(tones[i]);
	for ( i = 0; i < count; i++)
		free(signal[i]);

	char * p;
	temp[0] = '\0';
	p = (char *)wav_path;
	sprintf(temp, "\\adr%02dmod%dc%02d.wav", adresat, mode, count);
	p = append_string(p, temp);
	save_wav(signal_sum, num_total_samples, sample_rate, wav_path);

	free(signal_sum);
}

void gen_time_tones(float f0,float f1,float f2, float symbol_period, int signal_rate, float* signal) {
	int i;
	float* dphi, phi, n_ramp = 1200;

	dphi = (float*)malloc((12000*5)*sizeof(float));

	for ( i = 0; i < 12000; ++i) {
		dphi[i] = 2 * M_PI * f0 / signal_rate;       // first freq
	}
	phi = 0;
	for ( i = 0; i < 12000; ++i) {
		signal[i] = sinf(phi);
		phi = fmodf(phi + dphi[i], 2 * M_PI);
	}
	for ( i = 0; i < n_ramp; ++i) {                  // затухание с краёв
		float env = (1 - cosf( M_PI * i / n_ramp )) / 2;
		signal[i] *= env;
		signal[12000 - 1 - i] *= env;
	}
//-----------------
	for ( i=12000; i < 24000; ++i) {
		signal[i] = 0;                               // промежуток
	}
	for ( i = 12000; i < 24000; ++i) {
		dphi[i] = 2 * M_PI * 2222.0 / signal_rate;
	}
	phi = 0;
	for ( i = 12000; i < 24000; ++i) {
		signal[i] = sinf(phi);
		phi = fmodf(phi + dphi[i], 2 * M_PI);
	}
	for ( i = 0; i < n_ramp; ++i) {                  // затухание с краёв
		float env = (1 - cosf( M_PI * i / n_ramp )) / 2;
		signal[i+12000] *= env;
		signal[24000 - 1 - i] *= env;
	}
//-----------------
	for ( ; i < 36000; ++i) {
		dphi[i] = 2 * M_PI * f1 / signal_rate;
	}
	phi = 0;
	for ( i=24000; i < 36000; ++i) {
		signal[i] = sinf(phi);
		phi = fmodf(phi + dphi[i], 2 * M_PI);
	}
	for ( i = 0; i < n_ramp; ++i) {
		float env = (1 - cosf( M_PI * i / n_ramp )) / 2;
		signal[i+24000] *= env;
		signal[36000 - 1 - i] *= env;
	}
//-----------------
	for ( i=36000; i < 48000; ++i) {
		signal[i] = 0;
	}
	for ( i = 36000; i < 48000; ++i) {
		dphi[i] = 2 * M_PI * 2222.0 / signal_rate;
	}
	phi = 0;
	for ( i = 36000; i < 48000; ++i) {
		signal[i] = sinf(phi);
		phi = fmodf(phi + dphi[i], 2 * M_PI);
	}
	for ( i = 0; i < n_ramp; ++i) {                  // затухание с краёв
		float env = (1 - cosf( M_PI * i / n_ramp )) / 2;
		signal[i+36000] *= env;
		signal[48000 - 1 - i] *= env;
	}
//-----------------
	for ( ; i < 60000; ++i) {
		dphi[i] = 2 * M_PI * f2 / signal_rate;
	}
	phi = 0;
	for ( i=48000; i < 60000; ++i) {
		signal[i] = sinf(phi);
		phi = fmodf(phi + dphi[i], 2 * M_PI);
	}
	for ( i = 0; i < n_ramp; ++i) {
		float env = (1 - cosf( M_PI * i / n_ramp )) / 2;
		signal[i+48000] *= env;
		signal[60000 - 1 - i] *= env;
	}

	free(dphi);
}

void gen_time_sync( float f0, float f1, float f2, float* signal) {
	int l, i, j, num_tones;
	int sample_rate = 12000, num_samples, num_silence;
	char original[200];
	char temp[29];
	float symbol_period;
	bool is_ft4=false;

	symbol_period = (is_ft4) ? FT4_SYMBOL_PERIOD : FT8_SYMBOL_PERIOD;

	// Synthesize waveform data (signal) and save it as WAV file
	gen_time_tones(f0,f1,f2, symbol_period, sample_rate, signal);
}

