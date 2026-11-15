#ifndef _INCLUDE_COMMON_H_
#define _INCLUDE_COMMON_H_

#define MAX_MESS_QNT 32
#define COMMON_CALL 63
#define MAIN_KNOT 0
#define BEG_FREQ 400.0
#define ABONENTS_MAX 15
#define BAND_USED 2300
#define TF8_BAND_FREQ 72
#define TOTAL_DIVISIONS 43

#define ATTACHMENTS_MAX 32

#define ENG_VARIANT

typedef struct { char attachment[14]; int n; int sign_i3, to; float snr; float mes_freq; } s_attachment;

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
extern "C"
{
#endif

void hashtable_init(void);
char * decode_ft8(const char * path_file, int * sign_i3, float * snr, float * freq);
void gen_ft8( int adresat, int mode, int num_mes, const char * message, char * wav_path);
int FindFreqForDivision(int mode);
void gen_ft8_hor( int adresat, int mode, const char * message, float * wav_mass);
void gen_ft8_hor_to_file( int adresat, int mode, const char * message, char * wav_path);
void gen_time_sync( float f0, float f1, float f2, float* signal);

#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_COMMON_H_
