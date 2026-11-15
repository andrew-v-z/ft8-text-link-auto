#ifndef _INCLUDE_MESSAGE_H_
#define _INCLUDE_MESSAGE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define FTX_PAYLOAD_LENGTH_BYTES 10 ///< number of bytes to hold 77 bits of FTx payload data
#define FTX_MAX_MESSAGE_LENGTH   35 ///< max message length = callsign[13] + space + callsign[13] + space + report[6] + terminator

/// Structure that holds the decoded message
typedef struct {
    uint8_t payload[FTX_PAYLOAD_LENGTH_BYTES];
    uint16_t hash; ///< Hash value to be used in hash table and quick checking for duplicates
} ftx_message_t;

typedef enum {
	MESSAGE_REQ_SNR,
	MESSAGE_STANDART,
	MESSAGE_LONG,
	MESSAGE_SILENSE,
	MESSAGE_TIME,
	MESSAGE_UNKNOWN
} message_type_t;

typedef enum {
    FTX_MESSAGE_RC_OK,
    FTX_MESSAGE_RC_ERROR_CALLSIGN1,
    FTX_MESSAGE_RC_ERROR_CALLSIGN2,
	FTX_MESSAGE_RC_ERROR_SUFFIX,
    FTX_MESSAGE_RC_ERROR_GRID,
    FTX_MESSAGE_RC_ERROR_TYPE
} ftx_message_rc_t;

typedef enum {
	FTX_CALLSIGN_HASH_22_BITS,
	FTX_CALLSIGN_HASH_12_BITS,
	FTX_CALLSIGN_HASH_10_BITS
} ftx_callsign_hash_type_t;

// Callsign types and sizes:
// * Nonstd. call - all the rest, limited to 3-11 characters either alphanumeric or stroke (/)

void ftx_message_init(ftx_message_t* msg);

uint8_t ftx_message_get_i3(const ftx_message_t* msg);
uint8_t ftx_message_get_n3(const ftx_message_t* msg);
message_type_t message_get_type(const ftx_message_t* msg);

// bool ftx_message_check_recipient(const ftx_message_t* msg, const char* callsign);

/// Pack (encode) a text message
ftx_message_rc_t ftx_message_encode(ftx_message_t* msg, const char* message_text);
ftx_message_rc_t ftx_message_encode_nonstd(ftx_message_t* msg, const char* call_to, const char* call_n, const char* extra, const char* call_de);
ftx_message_rc_t ftx_message_decode(const ftx_message_t* msg, char* message, int * ii3, int * i_to);
ftx_message_rc_t ftx_message_decode_nonstd(const ftx_message_t* msg, char* call_to, char* call_n, char* extra, char* call_de);
////////////////////////////////////////////////////// Static function prototypes //////////////////////////////////////////////////////////////

/// Pack a non-standard base call into a 28-bit integer.
static bool pack58(const char* callsign, uint64_t* n58);

/// Unpack a non-standard base call from a 58-bit integer.
static bool unpack58(uint64_t n58, char* callsign);

/////////////////////////////////////////////////////////// Exported functions /////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_MESSAGE_H_
