#include "audio.h"

#define LOG_LEVEL LOG_INFO
#include <ft8/debug.h>

#include <stdio.h>
#include <string.h>

#define USE_PORTAUDIO
#ifdef USE_PORTAUDIO
#include <include/portaudio.h>

typedef struct {
    PaStream* instream;
} audio_context_t;

static audio_context_t audio_context;

static int audio_cb(void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
    audio_context_t* context = (audio_context_t*)userData;
    float* samples_in = (float*)inputBuffer;

    // PaTime time = data->startTime + timeInfo->inputBufferAdcTime;
    printf("Callback with %ld samples\n", framesPerBuffer);
	return 0;
}

void audio_list(void) {
	PaError pa_rc;
	int numDevices, i;
	PaStreamParameters inputParameters;
	double sample_rate;

	pa_rc = Pa_Initialize(); // Initialize PortAudio
	if (pa_rc != paNoError)     {
		printf("Error initializing PortAudio.\n");
		printf("\tErrortext: %s\n\tNumber: %d\n", Pa_GetErrorText(pa_rc), pa_rc);
		return;
	}

	numDevices = Pa_GetDeviceCount();
	if (numDevices < 0) {
		printf("ERROR: Pa_CountDevices returned 0x%x\n", numDevices);
		return;
	}

	printf("%d audio devices found:\n", numDevices);
	for ( i = 0; i < numDevices; i++) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);

		inputParameters.device = i,
		inputParameters.channelCount = 1, // 1 = mono, 2 = stereo
		inputParameters.sampleFormat = paFloat32,
		inputParameters.suggestedLatency = 0.2,
		inputParameters.hostApiSpecificStreamInfo = NULL;

		sample_rate = 12000; // sample rate (frames per second)
        pa_rc = Pa_IsFormatSupported(NULL, &inputParameters, sample_rate);
//        pa_rc = Pa_IsFormatSupported(&inputParameters, NULL, sample_rate);  // for input

        printf("%d: %s [%s]\n", (i + 1), (deviceInfo->name), (pa_rc == paNoError) ? "OK" : "NOT SUPPORTED");
    }
}

int audio_init(void) {
    PaError pa_rc;

	pa_rc = Pa_Initialize(); // Initialize PortAudio
    if (pa_rc != paNoError) {
        printf("Error initializing PortAudio.\n");
		printf("\tErrortext: %s\n\tNumber: %d\n", Pa_GetErrorText(pa_rc), pa_rc);
		Pa_Terminate(); // I don't think we need this but...
        return -1;
    }
	return 0;
}

int audio_open(const int dev_index, int in_out) {
	int i, numDevices;
	PaError pa_rc;
	PaStreamParameters Parameters;
	PaDeviceIndex ndevice_in = -1;
	PaDeviceIndex ndevice_out = -1;
	unsigned long nfpb = 1920 / 4; // frames per buffer
	double sample_rate = 12000;    // sample rate (frames per second)

	numDevices = Pa_GetDeviceCount();
	audio_context.instream = NULL;

	for ( i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
		if (in_out==0) {
			if (dev_index == i) {
				ndevice_in = i;
				break;
			}
		}else{
			if (dev_index == i) {
				ndevice_out = i;
				break;
			}
		}
	}

	if (((in_out==0) && (ndevice_in < 0)) || ((in_out==1) && (ndevice_out < 0))) {
//		printf("Could not find device [%s].\n", name);
//		audio_list();
		return -1;
	}

	if (in_out==0) {
		Parameters.device = ndevice_in;
	} else {
		Parameters.device = ndevice_out;
	}
	Parameters.channelCount = 1; // 1 = mono, 2 = stereo
	Parameters.sampleFormat = paFloat32;
	Parameters.suggestedLatency = 0.2;
	Parameters.hostApiSpecificStreamInfo = NULL;

	// Test if this configuration actually works, so we do not run into an ugly assertion
	if (in_out==0) {
		pa_rc = Pa_IsFormatSupported(&Parameters, NULL, sample_rate);
	} else {
		pa_rc = Pa_IsFormatSupported(NULL, &Parameters, sample_rate);
	}
	if (pa_rc != paNoError) {
		printf("Error opening input audio stream.\n");
		printf("\tErrortext: %s\n\tNumber: %d\n", Pa_GetErrorText(pa_rc), pa_rc);
		return -2;
	}

	PaStream* instream;
	if (in_out==0) {
	  pa_rc = Pa_OpenStream(
		&instream, // address of stream
		&Parameters,
		NULL,
		sample_rate, // Sample rate
		nfpb,        // Frames per buffer
		paNoFlag,
		NULL /*(PaStreamCallback*)audio_cb*/, // Callback routine
		NULL /*(void*)&audio_context*/);      // address of data structure
	}else{
	  pa_rc = Pa_OpenStream(
		&instream, // address of stream
		NULL,
		&Parameters,
		sample_rate, // Sample rate
		nfpb,        // Frames per buffer
		paNoFlag,
		NULL, // Callback routine
		NULL);      // address of data structure
	}
	if (pa_rc != paNoError) { // We should have no error here usually
		printf("Error opening input audio stream:\n");
		printf("\tErrortext: %s\n\tNumber: %d\n", Pa_GetErrorText(pa_rc), pa_rc);
		return -3;
	}
//	printf("Successfully opened audio input.\n");

	pa_rc = Pa_StartStream(instream); // Start input stream
	if (pa_rc != paNoError) {
		printf("Error starting input audio stream!\n");
		printf("\tErrortext: %s\n\tNumber: %d\n", Pa_GetErrorText(pa_rc), pa_rc);
		return -4;
	}

	audio_context.instream = instream;

	return 0;
}

/*
int audio_open(const char* name) {
	int i, numDevices;
	PaError pa_rc;
	PaStreamParameters inputParameters;
	PaDeviceIndex ndevice_in = -1;
	unsigned long nfpb = 1920 / 4; // frames per buffer
	double sample_rate = 12000;    // sample rate (frames per second)

	numDevices = Pa_GetDeviceCount();
	audio_context.instream = NULL;

	for ( i = 0; i < numDevices; i++) {
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
		if (CurrentInput == i) {
//		if (0 == strcmp(deviceInfo->name, name)) {
			ndevice_in = i;
			break;
		}
	}

	if (ndevice_in < 0) {
		printf("Could not find device [%s].\n", name);
		audio_list();
		return -1;
	}

	inputParameters.device = ndevice_in,
	inputParameters.channelCount = 1, // 1 = mono, 2 = stereo
	inputParameters.sampleFormat = paFloat32,
	inputParameters.suggestedLatency = 0.2,
	inputParameters.hostApiSpecificStreamInfo = NULL;

	// Test if this configuration actually works, so we do not run into an ugly assertion
	pa_rc = Pa_IsFormatSupported(&inputParameters, NULL, sample_rate);
	if (pa_rc != paNoError) {
		printf("Error opening input audio stream.\n");
		printf("\tErrortext: %s\n\tNumber: %d\n", Pa_GetErrorText(pa_rc), pa_rc);
		return -2;
	}

	PaStream* instream;
	pa_rc = Pa_OpenStream(
		&instream, // address of stream
		&inputParameters,
		NULL,
		sample_rate, // Sample rate
		nfpb,        // Frames per buffer
		paNoFlag,
		NULL, // Callback routine (PaStreamCallback*)audio_cb
		NULL );      // address of data structure (void*)&audio_context
	if (pa_rc != paNoError) { // We should have no error here usually
		printf("Error opening input audio stream:\n");
		printf("\tErrortext: %s\n\tNumber: %d\n", Pa_GetErrorText(pa_rc), pa_rc);
		return -3;
	}
	printf("Successfully opened audio input.\n");

	pa_rc = Pa_StartStream(instream); // Start input stream
	if (pa_rc != paNoError) {
		printf("Error starting input audio stream!\n");
		printf("\tErrortext: %s\n\tNumber: %d\n", Pa_GetErrorText(pa_rc), pa_rc);
		return -4;
	}

	audio_context.instream = instream;

	// while (Pa_IsStreamActive(instream))
	// {
	//     Pa_Sleep(100);
	// }
	// Pa_AbortStream(instream); // Abort stream
	// Pa_CloseStream(instream); // Close stream, we're done.

	return 0;
}
*/
int audio_read(float* buffer, int num_samples) {
	PaError pa_rc;
	pa_rc = Pa_ReadStream(audio_context.instream, (void*)buffer, num_samples);
	return 0;
}

int audio_write(float* buffer, int num_samples) {
	return Pa_WriteStream(audio_context.instream, (void*)buffer, num_samples);
}

#else

int audio_init(void) {
    return -1;
}

void audio_list(void) {
}

int audio_open(const char* name, int in_out) {
    return -1;
}

int audio_read(float* buffer, int num_samples) {
    return -1;
}

#endif