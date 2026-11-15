#include "message.h"
#include "text.h"
#include <stdlib.h>
#include <string.h>

#define LOG_LEVEL LOG_INFO
#include "debug.h"

#define MAX22    ((uint32_t)4194304ul)
#define NTOKENS  ((uint32_t)2063592ul)
#define MAXGRID4 ((uint16_t)32400ul)

#define MAX_B	13

void ftx_message_init(ftx_message_t* msg) {
    memset((void*)msg, 0, sizeof(ftx_message_t));
}

message_type_t message_get_type(const ftx_message_t* msg) {
    // Extract i3
    uint8_t i3 = (uint8_t)(msg->payload[1] >> 4) & 0x03u;

	switch (i3) {
		case 0:
			return MESSAGE_REQ_SNR;
		case 1:
			return MESSAGE_STANDART;
		case 2:
			return MESSAGE_LONG;
		case 3:
			return MESSAGE_SILENSE;
		default:
			return MESSAGE_UNKNOWN;
	}
}

ftx_message_rc_t ftx_message_encode(ftx_message_t* msg, const char* message_text){
    char call_to[3];
    char call_n[3];
    char call_de[MAX_B];
    char extra[2];
    ftx_message_rc_t rc;

	strncpy(call_to, message_text,   3);
	call_to[2] = '\0';
	strncpy(call_n,  message_text+3, 3);
	call_n[2] = '\0';
	strncpy(extra,   message_text+6, 2);
	extra[1] = '\0';
	strncpy(call_de, message_text+8, 13);
	call_de[12] = '\0';

	rc = ftx_message_encode_nonstd(msg, call_to, call_n, extra, call_de);
	if (rc == FTX_MESSAGE_RC_OK)     return rc;

    return rc;
}

ftx_message_rc_t ftx_message_encode_nonstd(ftx_message_t* msg, const char* call_to, const char* call_n, const char* extra, const char* call_de) {

	uint8_t i3 = atoi(extra);
	uint64_t n58;
	uint8_t  n = atoi(call_n);
	uint8_t to = atoi(call_to);
	const char* call58 = call_de;

	if (!pack58(call58, &n58))
		return FTX_MESSAGE_RC_ERROR_CALLSIGN2;

	// Pack into     13 + 58 + 7 == 78 bits
	msg->payload[0] = (uint8_t)(to << 2) | (uint8_t)(n >> 3);
	msg->payload[1] = (uint8_t)(uint8_t)(n << 5) | (i3 << 3) | (uint8_t)(n58 >> 61);
	msg->payload[2] = (uint8_t)(n58 >> 53);
	msg->payload[3] = (uint8_t)(n58 >> 45);
	msg->payload[4] = (uint8_t)(n58 >> 37);
	msg->payload[5] = (uint8_t)(n58 >> 29);
	msg->payload[6] = (uint8_t)(n58 >> 21);
	msg->payload[7] = (uint8_t)(n58 >> 13);
	msg->payload[8] = (uint8_t)(n58 >> 5);
	msg->payload[9] = (uint8_t)(n58 << 3);

/*    printf("Packed =====eee==========: ");
	for (int j = 0; j < 10; ++j) {
		printf("%02x ", (uint64_t)msg->payload[j]);
	}
	printf("\n");
*/
	return FTX_MESSAGE_RC_OK;
}

ftx_message_rc_t ftx_message_decode(const ftx_message_t* msg, char* message, int * ii3, int * i_to) {
	ftx_message_rc_t rc;
	message_type_t msg_type = message_get_type(msg);

	char buf[35]; // 3 + 3 + 2 + 14
	char* to       = buf;
	char* nom      = buf + 3;
	char* extra_i3 = buf + 3 + 3;
	char* text     = buf + 3 + 3 + 2;

	message[0] = '\0';

	switch (msg_type) {
	case MESSAGE_REQ_SNR:
	case MESSAGE_STANDART:
	case MESSAGE_LONG:
	case MESSAGE_SILENSE:
		rc = ftx_message_decode_nonstd(msg, to, nom, extra_i3, text);
		*ii3 = dd_to_int(extra_i3, 2);
		*i_to = dd_to_int(to, 3);
		break;
	case MESSAGE_UNKNOWN:
	default:
		// not handled yet
		break;
	}

	if (to != NULL) {
        // TODO join fields via whitespace
		message = append_string(message, to);
		if (nom != NULL) {
            message = append_string(message, " ");
			message = append_string(message, nom);
			if (extra_i3 != NULL) {
                message = append_string(message, " ");
				message = append_string(message, extra_i3);
				if (text != NULL) {
                    message = append_string(message, " ");
					message = append_string(message, text);
                }
            }
        }
    }

    return rc;
}

// non-standard messages, code originally by KD8CEC
ftx_message_rc_t ftx_message_decode_nonstd(const ftx_message_t* msg, char* call_to, char* call_n, char* extra, char* call_de) {
	uint8_t to = (msg->payload[0] >> 2);    // 6 bit
	uint8_t n  =((msg->payload[0] << 3) & 0x1Fu) | (msg->payload[1] >> 5); // 5 bit
	uint8_t i3 = (msg->payload[1] >> 3) & 3;  // 2 bit

	uint64_t n58;
	n58 =  ((uint64_t)(msg->payload[1] & 0x07u) << 61);
	n58 |= ((uint64_t)msg->payload[2] << 53);
	n58 |= ((uint64_t)msg->payload[3] << 45);
	n58 |= ((uint64_t)msg->payload[4] << 37);
	n58 |= ((uint64_t)msg->payload[5] << 29);
	n58 |= ((uint64_t)msg->payload[6] << 21);
	n58 |= ((uint64_t)msg->payload[7] << 13);
    n58 |= ((uint64_t)msg->payload[8] << 5);
	n58 |= ((uint64_t)msg->payload[9] >> 3);

	sprintf(call_to, "%02d", to);
    sprintf(call_n, "%02d", n);
    sprintf(extra, "%d", i3);

    // Decode one of the calls from 58 bit encoded string
    unpack58(n58, call_de);

    LOG(LOG_DEBUG, "Decoded non-standard (typ %d) message [%s] [%s] [%s]\n", i3, call_to, call_n, call_de);
    return FTX_MESSAGE_RC_OK;
}

/////////////////////////////////////////////////////////// Static functions /////////////////////////////////////////////////////////////////

static bool pack58(const char* callsign, uint64_t* n58) {
	// Decode one of the calls from 58 bit encoded string
	const char* src = callsign;
	uint64_t result = 0;
	char c11[MAX_B];
	int length = 0;

	while (*src != '\0' && (length < (MAX_B-1))) {
        int j = nchar(*src, FT8_CHAR_TABLE_ALPHANUM_SPACE_SLASH);
		c11[length] = *src;
		if (j < 0)
            return false;
        result = (result * 38) + j;
        src++;
        length++;
    }
    c11[length] = '\0';

    *n58 = result;
    LOG(LOG_DEBUG, "pack58('%s')=%016llx\n", callsign, *n58);
    return true;
}

static bool unpack58(uint64_t n58, char* callsign) {
    // Decode one of the calls from 58 bit encoded string
    uint64_t n58_backup = n58;
	char c11[MAX_B];
	int i;

	c11[MAX_B-1] = '\0';
	for ( i = MAX_B-2;; --i) {
        c11[i] = charn(n58 % 38, FT8_CHAR_TABLE_ALPHANUM_SPACE_SLASH);
        if (i == 0)   break;
        n58 /= 38;
    }
    // The decoded string will be right-aligned, so trim all whitespace (also from back just in case)
    strcpy(callsign,c11);

    LOG(LOG_DEBUG, "unpack58(%016llx)=%s\n", n58_backup, callsign);

    return false;
}

