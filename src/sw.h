#pragma once

/**
 * Status word for success.
 */
#define SW_OK 0x9000
/**
 * Status word for denied by user.
 */
#define SW_DENY 0x6985
/**
 * Status word for incorrect P1 or P2.
 */
#define SW_WRONG_P1P2 0x6A86
/**
 * Status word for either wrong Lc or lenght of APDU command less than 5.
 */
#define SW_WRONG_APDU_DATA_LENGTH 0x6A87
/**
 * Status word for unknown command with this INS.
 */
#define SW_INS_NOT_SUPPORTED 0x6D00
/**
 * Status word for instruction class is different than CLA.
 */
#define SW_CLA_NOT_SUPPORTED 0x6E00
/**
 * Status word for busy state.
 */
#define SW_BUSY 0xB000
/**
 * Status word for wrong reponse length (buffer too small or too big).
 */
#define SW_WRONG_RESPONSE_LENGTH 0xB001

#define SW_BAD_SESSION_ID 0xB002

#define SW_WRONG_SUBCOMMAND 0xB003

#define SW_BAD_STATE 0xB0FF

#define SW_BAD_TOKEN_ID               0xE001
#define SW_BAD_TOKEN_VALUE            0xE002
#define SW_BAD_CONTEXT_EXTENSION_SIZE 0xE003
#define SW_BAD_DATA_INPUT             0xE004
#define SW_BAD_BOX_ID                 0xE005
#define SW_BAD_TOKEN_INDEX            0xE006
#define SW_BAD_FRAME_INDEX            0xE007
#define SW_BAD_INPUT_COUNT            0xE008
#define SW_BAD_OUTPUT_COUNT           0xE009
#define SW_TOO_MANY_TOKENS            0xE00A
#define SW_TOO_MANY_INPUTS            0xE00B
#define SW_TOO_MANY_DATA_INPUTS       0xE00C
#define SW_TOO_MANY_INPUT_FRAMES      0xE00D
#define SW_TOO_MANY_OUTPUTS           0xE00E
#define SW_TOO_MANY_REGISTERS         0xE00F
#define SW_HASHER_ERROR               0xE010
#define SW_BUFFER_ERROR               0xE011
#define SW_U64_OVERFLOW               0xE012
#define SW_BIP32_BAD_PATH             0xE013
#define SW_INTERNAL_CRYPTO_ERROR      0xE014
#define SW_NOT_ENOUGH_DATA            0xE015
#define SW_TOO_MUCH_DATA              0xE016
#define SW_ADDRESS_GENERATION_FAILED  0xE017
#define SW_BAD_TRANSACTION_PREFIX_LEN 0xE018
#define SW_BAD_TRANSACTION_SUFFIX_LEN 0xE019
#define SW_HMAC_ERROR                 0xE01A
#define SW_BAD_FRAME_SIGNATURE        0xE01B

#define SW_BIP32_FORMATTING_FAILED   0xE101
#define SW_ADDRESS_FORMATTING_FAILED 0xE102

#define SW_STACK_OVERFLOW 0xFFFF
