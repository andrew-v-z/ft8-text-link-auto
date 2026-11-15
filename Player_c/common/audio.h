#ifndef _INCLUDE_AUDIO_H_
#define _INCLUDE_AUDIO_H_

#ifdef __cplusplus
extern "C"
{
#endif

int audio_init(void);
void audio_list(void);
int audio_open(const int dev_index, int in_out);
int audio_read(float* buffer, int num_samples);
int audio_write(float* buffer, int num_samples);

#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_AUDIO_H_