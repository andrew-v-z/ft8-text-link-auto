#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#include <ft8/decode.h>
#include <ft8/encode.h>
#include <ft8/message.h>
#include <ft8/text.h>

#include <common/common.h>
#include <common/wave.h>
#include <common/monitor.h>
#include <common/audio.h>

const int kMin_score = 10; // Minimum sync score threshold for candidates
const int kMax_candidates = 140;
const int kLDPC_iterations = 25;

const int kMax_decoded_messages = 50;

const int kFreq_osr = 2; // Frequency oversampling rate (bin subdivision)
const int kTime_osr = 2; // Time oversampling rate (symbol subdivision)

char bf[256];

void usage(const char* error_msg) {
    if (error_msg != NULL) {
        fprintf(stderr, "ERROR: %s\n", error_msg);
    }
	fprintf(stderr, "Usage: decode_ft8 [WAV file | -list | ([-ft4] [INPUT|-dev DEVICE])]\n");
}

#define CALLSIGN_HASHTABLE_SIZE 256

static struct {
    char callsign[12]; ///> Up to 11 symbols of callsign + trailing zeros (always filled)
    uint32_t hash;     ///> 8 MSBs contain the age of callsign; 22 LSBs contain hash value
} callsign_hashtable[CALLSIGN_HASHTABLE_SIZE];

static int callsign_hashtable_size;

void hashtable_init(void) {
    callsign_hashtable_size = 0;
    memset(callsign_hashtable, 0, sizeof(callsign_hashtable));
}

void hashtable_cleanup(uint8_t max_age) {
	int idx_hash;
	for ( idx_hash = 0; idx_hash < CALLSIGN_HASHTABLE_SIZE; ++idx_hash) {
        if (callsign_hashtable[idx_hash].callsign[0] != '\0') {
            uint8_t age = (uint8_t)(callsign_hashtable[idx_hash].hash >> 24);
            if (age > max_age) {
				// free the hash entry
                callsign_hashtable[idx_hash].callsign[0] = '\0';
                callsign_hashtable[idx_hash].hash = 0;
                callsign_hashtable_size--;
            } else {
                // increase callsign age
                callsign_hashtable[idx_hash].hash = (((uint32_t)age + 1u) << 24) | (callsign_hashtable[idx_hash].hash & 0x3FFFFFu);
            }
        }
    }
}

void hashtable_add(const char* callsign, uint32_t hash) {
    uint16_t hash10 = (hash >> 12) & 0x3FFu;
    int idx_hash = (hash10 * 23) % CALLSIGN_HASHTABLE_SIZE;
	while (callsign_hashtable[idx_hash].callsign[0] != '\0') {
        if (((callsign_hashtable[idx_hash].hash & 0x3FFFFFu) == hash) &&
             (0 == strcmp(callsign_hashtable[idx_hash].callsign, callsign))) {
            // reset age
            callsign_hashtable[idx_hash].hash &= 0x3FFFFFu;
			return;
        } else {
			// Move on to check the next entry in hash table
            idx_hash = (idx_hash + 1) % CALLSIGN_HASHTABLE_SIZE;
        }
    }
    callsign_hashtable_size++;
    strncpy(callsign_hashtable[idx_hash].callsign, callsign, 11);
    callsign_hashtable[idx_hash].callsign[11] = '\0';
    callsign_hashtable[idx_hash].hash = hash;
}

bool hashtable_lookup(ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign) {
    uint8_t hash_shift = (hash_type == FTX_CALLSIGN_HASH_10_BITS) ? 12 : (hash_type == FTX_CALLSIGN_HASH_12_BITS ? 10 : 0);
    uint16_t hash10 = (hash >> (12 - hash_shift)) & 0x3FFu;
    int idx_hash = (hash10 * 23) % CALLSIGN_HASHTABLE_SIZE;
    while (callsign_hashtable[idx_hash].callsign[0] != '\0') {
        if (((callsign_hashtable[idx_hash].hash & 0x3FFFFFu) >> hash_shift) == hash) {
            strcpy(callsign, callsign_hashtable[idx_hash].callsign);
            return true;
        }
        // Move on to check the next entry in hash table
        idx_hash = (idx_hash + 1) % CALLSIGN_HASHTABLE_SIZE;
    }
    callsign[0] = '\0';
    return false;
}

void decode(const monitor_t* mon, s_attachment * letter_out) {
	int i, idx;
	const ftx_waterfall_t* wf = &mon->wf;
	char text[FTX_MAX_MESSAGE_LENGTH];
	float snr = 0;

	// Find top candidates by Costas sync score and localize them in time and frequency
	ftx_candidate_t * candidate_list = (ftx_candidate_t*)malloc(kMax_candidates*sizeof(ftx_candidate_t));
//	ftx_candidate_t candidate_list[kMax_candidates];
	int num_candidates = ftx_find_candidates(wf, kMax_candidates, candidate_list, kMin_score);

	// table for decoded messages (to check for duplicates)
	int num_decoded = 0, num = 0;
	ftx_message_t * decoded = (ftx_message_t *)malloc(kMax_decoded_messages*sizeof(ftx_message_t));
//	ftx_message_t decoded[kMax_decoded_messages];
//	ftx_message_t ** decoded_hashtable = (ftx_message_t **)malloc(kMax_decoded_messages*sizeof(ftx_message_t*));
	ftx_message_t* decoded_hashtable[kMax_decoded_messages];

	// Initialize hash table pointers
	for ( i = 0; i < kMax_decoded_messages; ++i) {
		decoded_hashtable[i] = NULL;
	}

	// Go over candidates and attempt to decode messages
	for ( idx = 0; idx < num_candidates; ++idx) {
		int idx_hash;
		bool found_empty_slot = false;
		bool found_duplicate = false;
		const ftx_candidate_t* cand = &candidate_list[idx];

		float freq_hz  = (mon->min_bin + cand->freq_offset + (float)cand->freq_sub / wf->freq_osr) / mon->symbol_period;
		float time_sec = (cand->time_offset + (float)cand->time_sub / wf->time_osr) * mon->symbol_period;

		ftx_message_t message;
		ftx_decode_status_t status;
		if (!ftx_decode_candidate(wf, cand, kLDPC_iterations, &message, &status)) {
//			if (status.ldpc_errors > 0) {
//				printf("LDPC decode: %d errors\n", status.ldpc_errors);
//			} else if (status.crc_calculated != status.crc_extracted) {
//				printf("CRC mismatch!\n");
//			}
			continue;
		}

//		printf( "Checking table for %4.1fs / %4.1fHz [%d]...\n", time_sec, freq_hz, cand->score);
		idx_hash = message.hash % kMax_decoded_messages;
		do {
			if (decoded_hashtable[idx_hash] == NULL) {
				found_empty_slot = true;
			}  else if ((decoded_hashtable[idx_hash]->hash == message.hash) &&
						(0 == memcmp(decoded_hashtable[idx_hash]->payload, message.payload, sizeof(message.payload)))) {
				found_duplicate = true;
			} else {
				// Move on to check the next entry in hash table
				idx_hash = (idx_hash + 1) % kMax_decoded_messages;
			}
		} while (!found_empty_slot && !found_duplicate);

		if (found_empty_slot) {
			ftx_message_rc_t unpack_status;
			memcpy(&decoded[idx_hash], &message, sizeof(message));
			decoded_hashtable[idx_hash] = &decoded[idx_hash];
			++num_decoded;

			int ii3 = 0, to;
			unpack_status = ftx_message_decode(&message, text, &ii3, &to);
			if (unpack_status != FTX_MESSAGE_RC_OK) {
				snprintf(text, sizeof(text), "Error [%d] while unpacking!", (int)unpack_status);
//				strcpy(letter_out, text);
			} else {
				int part;
				char buf[14];
				strncpy(buf, text + 3, 2);
				buf[2] = '\0';
				part = atoi(buf);
				letter_out[num].n 		= part;   	// partnum of long message

				letter_out[num].sign_i3 = ii3;		// sign_i3

				letter_out[num].to		= to;		// to - adresat

				letter_out[num].snr = cand->score;  // snr
/*
				uint8_t tones[FT8_NN];
				ft8_encode(message.payload, tones);
				letter_out[num].snr = compute_true_snr(&mon->wf, cand, tones);
*/
				letter_out[num].mes_freq = freq_hz; // freq
				strncpy(buf, text + 8, 13);
				buf[13] = '\0';
				strcpy(letter_out[num++].attachment, buf ); // text of message
			}

			// Fake WSJT-X-like output for now
			//snr = cand->score; // TODO: compute better approximation of SNR
		}
	}
//	printf("Decoded %d messages, callsign hashtable size %d\n", num_decoded, callsign_hashtable_size);

//	if (num_decoded > 0) {
//		sprintf(text, " (%.1f dB %.1f dB)", mon->max_mag, snr/num_decoded-33.1);
//		*snr_out = snr/num_decoded-33.1;
//	}

	hashtable_cleanup(10);
	free(decoded);
	free(candidate_list);
}

char * decode_ft8(const char * path_file, int * sign_i3, float * snr, float * freq) {
	extern char letter[400];
	s_attachment attachments[ATTACHMENTS_MAX] = {0};
	float slot_period;
	ftx_protocol_t protocol = FTX_PROTOCOL_FT8;
	float time_shift = 0.8;
	int sample_rate = 12000,num_samples;
	monitor_t mon;
	monitor_config_t mon_cfg;

	letter[0] = '\0';
	slot_period = ((protocol == FTX_PROTOCOL_FT8) ? FT8_SLOT_TIME : FT4_SLOT_TIME);
	num_samples = slot_period * sample_rate;
	float * signal = (float*)malloc(num_samples*sizeof(float));
//	float signal[num_samples];

	if (strlen(path_file)>0) {
		int rc = load_wav(signal, &num_samples, &sample_rate, path_file);
		if (rc < 0) {
//			wsprintfA(bf, "ERROR: cannot load wave file %s", path_file);
			return 0;
		}
//		printf("Sample rate %d Hz, %d samples, %.3f seconds\n", sample_rate, num_samples, (double)num_samples / sample_rate);
	}

	// Compute FFT over the whole signal and store it
	mon_cfg.f_min = 200;
	mon_cfg.f_max = 3000;
	mon_cfg.sample_rate = sample_rate;
	mon_cfg.time_osr = kTime_osr;
	mon_cfg.freq_osr = kFreq_osr;
	mon_cfg.protocol = protocol;

	hashtable_init();

	monitor_init(&mon, &mon_cfg);

	// Process and accumulate audio data in a monitor/waterfall instance
	for (int frame_pos = 0; frame_pos + mon.block_size <= num_samples; frame_pos += mon.block_size) {
		monitor_process(&mon, signal + frame_pos);
	}
	// Decode accumulated data (containing slightly less than a full time slot)
	decode(&mon, attachments);

	// Reset internal variables for the next time slot
	monitor_reset(&mon);
	monitor_free(&mon);
	free(signal);

	int num_decoded = 0, count_parts = 0, sign;
	char * p, text[22] = {'\0'}, buff[256], long_letter[ATTACHMENTS_MAX][14] = {0};
	float snr_common = 0, cur_mes_freq;
	p = letter;
	while ( (num_decoded<ATTACHMENTS_MAX) && (strlen(attachments[num_decoded].attachment)>0)) {
		sign = attachments[num_decoded].sign_i3;
		cur_mes_freq = attachments[num_decoded].mes_freq;
		strcpy(long_letter[attachments[num_decoded].n], attachments[num_decoded].attachment );
		snr_common += attachments[num_decoded].snr;
		count_parts++;
		num_decoded++;
	}
	if (count_parts > 0) {
		for ( int i=0; i<ATTACHMENTS_MAX; i++) {
			p = append_string(p, long_letter[i]);
		}
		*sign_i3 = sign;
		*freq = cur_mes_freq;
		*snr = snr_common/count_parts;
		p = append_string(p, text);
	}

	return letter;
}

/*int main(int argc, char** argv) {
	// Accepted arguments
	const char* wav_path = NULL;
	const char* dev_name = NULL;
	float slot_period;
	ftx_protocol_t protocol = FTX_PROTOCOL_FT8;
	float time_shift = 0.8;
	int sample_rate = 12000,num_samples;
	float *signal;
	bool is_live = false;
	monitor_t mon;
	monitor_config_t mon_cfg;

	// Parse arguments one by one
	int arg_idx = 1;
	while (arg_idx < argc) {
		// Check if the current argument is an option (-xxx)
		if (argv[arg_idx][0] == '-') {
			// Check agaist valid options
			if (0 == strcmp(argv[arg_idx], "-ft4")) {
				protocol = FTX_PROTOCOL_FT4;
			} else if (0 == strcmp(argv[arg_idx], "-list"))  {
				audio_init();
				audio_list();
				return 0;
			} else if (0 == strcmp(argv[arg_idx], "-dev"))   {
				if (arg_idx + 1 < argc) {
					++arg_idx;
					dev_name = argv[arg_idx];
				} else  {
					usage("Expected an audio device name after -dev");
                    return -1;
                }
			} else {
                usage("Unknown command line option");
                return -1;
            }
		} else {
			if (wav_path == NULL) {
                wav_path = argv[arg_idx];
            } else {
                usage("Multiple positional arguments");
                return -1;
            }
        }
        ++arg_idx;
    }
    // Check if all mandatory arguments have been received
    if (wav_path == NULL && dev_name == NULL) {
        usage("Expected either INPUT file path or DEVICE name");
        return -1;
    }

	slot_period = ((protocol == FTX_PROTOCOL_FT8) ? FT8_SLOT_TIME : FT4_SLOT_TIME);
	num_samples = slot_period * sample_rate;
	signal = (float*)malloc(num_samples*sizeof(float));
//	float signal[num_samples];

    if (wav_path != NULL) {
        int rc = load_wav(signal, &num_samples, &sample_rate, wav_path);
        if (rc < 0) {
            LOG(LOG_ERROR, "ERROR: cannot load wave file %s\n", wav_path);
            return -1;
        }
        LOG(LOG_INFO, "Sample rate %d Hz, %d samples, %.3f seconds\n", sample_rate, num_samples, (double)num_samples / sample_rate);
	} else if (dev_name != NULL) {
		audio_init();
		audio_open(dev_name);
		num_samples = (slot_period - 0.4f) * sample_rate;
		is_live = true;
	}

    // Compute FFT over the whole signal and store it
		mon_cfg.f_min = 200;
		mon_cfg.f_max = 3000;
		mon_cfg.sample_rate = sample_rate;
		mon_cfg.time_osr = kTime_osr;
		mon_cfg.freq_osr = kFreq_osr;
		mon_cfg.protocol = protocol;

    hashtable_init();

    monitor_init(&mon, &mon_cfg);
    LOG(LOG_INFO, "Waterfall allocated %d symbols\n", mon.wf.max_blocks);

	do {
		int frame_pos;
        struct tm tm_slot_start = { 0 };
		if (is_live) {
            // Wait for the start of time slot
            while (true) {
//                struct timespec spec;
//                clock_gettime(CLOCK_REALTIME, &spec);
				double time = (double)13 + (35634562 / 1e9);
				double time_within_slot = fmod(time - time_shift, slot_period);
                if (time_within_slot > slot_period / 4) {
					audio_read(signal, mon.block_size);
                } else {
                    time_t time_slot_start = (time_t)(time - time_within_slot);
                    gmtime_r(&time_slot_start, &tm_slot_start);
                    LOG(LOG_INFO, "Time within slot %02d%02d%02d: %.3f s\n", tm_slot_start.tm_hour,
                        tm_slot_start.tm_min, tm_slot_start.tm_sec, time_within_slot);
                    break;
                }
            }
        }

        // Process and accumulate audio data in a monitor/waterfall instance
        for ( frame_pos = 0; frame_pos + mon.block_size <= num_samples; frame_pos += mon.block_size) {
            if (dev_name != NULL) {
                audio_read(signal + frame_pos, mon.block_size);
			}
			// LOG(LOG_DEBUG, "Frame pos: %.3fs\n", (float)(frame_pos + mon.block_size) / sample_rate);
			fprintf(stderr, "#");
			// Process the waveform data frame by frame - you could have a live loop here with data from an audio device
			monitor_process(&mon, signal + frame_pos);
		}
		fprintf(stderr, "\n");
		LOG(LOG_DEBUG, "Waterfall accumulated %d symbols\n", mon.wf.num_blocks);
		LOG(LOG_INFO, "Max magnitude: %.1f dB\n", mon.max_mag);

		// Decode accumulated data (containing slightly less than a full time slot)
		decode(&mon, &tm_slot_start);

		// Reset internal variables for the next time slot
		monitor_reset(&mon);
	} while (is_live);

	monitor_free(&mon);
	free(signal);
	return 0;
} */
