#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#include "ft8/text.h"
#include "ft8/encode.h"
#include "ft8/constants.h"

#include "fft/kiss_fftr.h"
#include "common/common.h"
#include "ft8/message.h"

#define LOG_LEVEL LOG_INFO
#include "ft8/debug.h"

#define CHECK(condition)                                       \
    if (!(condition))                                          \
    {                                                          \
        printf("FAIL: Condition \'" #condition "' failed!\n\n"); \
        return;                                                \
    }

#define TEST_END printf("Test OK\n\n")

char mmm[43];

void test_nonstd_msg(const char* mess_txt) {
	ftx_message_rc_t rc_encode, rc_decode;
	ftx_message_t msg;
	char message[35];
	int m1, m2;

	ftx_message_init(&msg);

	rc_encode = ftx_message_encode(&msg, mess_txt);
	CHECK(rc_encode == FTX_MESSAGE_RC_OK);

//	printf("Encoded [%s]\n", mess_txt);

	rc_decode = ftx_message_decode(&msg, message, &m1, &m2);
	CHECK(rc_decode == FTX_MESSAGE_RC_OK);
	printf("Decoded [%s]\n\n", message);
	CHECK(0 == strcmp(mess_txt, message));

	strcpy(mmm, message);
//    printf("\n");
    TEST_END;
}

#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))

char * test() {
    test_nonstd_msg("55 01 1 ABCDE 12345 ");
    test_nonstd_msg("30 02 1 ABCDEF 23456");
	test_nonstd_msg("10 07 1 024  12.41. ");
	return mmm;
}
