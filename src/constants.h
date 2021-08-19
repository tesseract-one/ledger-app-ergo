#pragma once

/**
 * Instruction class of the Ergo application.
 */
#define CLA 0xE0

/**
 * Length of APPNAME variable in the Makefile.
 */
#define APPNAME_LEN (sizeof(APPNAME) - 1)

/**
 * Maximum length of MAJOR_VERSION || MINOR_VERSION || PATCH_VERSION.
 */
#define APPVERSION_LEN 3

/**
 * Maximum length of application name.
 */
#define MAX_APPNAME_LEN 64

/**
 * Bip32 coin type of Ergo token
 */
#define BIP32_ERGO_COIN 429

/**
 * Length of Transaction Id.
 */
#define TRANSACTION_ID_LEN 32

/**
 * Length of Token Id.
 */
#define TOKEN_ID_LEN 32

/**
 * Maximum number of tokens in TX.
 */
#define TOKEN_MAX_COUNT 10

/**
 * Length of Transaction Hash.
 */
#define TRANSACTION_HASH_LEN 32

/**
 * Length of Box Id.
 */
#define BOX_ID_LEN 32

/**
 * Length of Session Key.
 */
#define SESSION_KEY_LEN 16

/**
 * Length of Chain Code.
 */
#define CHAIN_CODE_LEN 32

/**
 * Length of Public Key.
 */
#define PUBLIC_KEY_LEN 65

/**
 * Length of Address in bytes.
 */
#define ADDRESS_LEN 38

/**
 * Length of Address string in chars.
 */
#define ADDRESS_STRING_MAX_LEN 55

/**
 * Length of Extended Public Key.
 */
#define EXTENDED_PUBLIC_KEY_LEN (PUBLIC_KEY_LEN + CHAIN_CODE_LEN)
