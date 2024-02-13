#pragma once

#include <parser.h>

/**
 * Enumeration with expected INS of APDU commands.
 */
typedef enum {
    CMD_NONE = 0x00,                     /// empty command
    CMD_GET_APP_VERSION = 0x01,          /// version of the application
    CMD_GET_APP_NAME = 0x02,             /// application name
    CMD_GET_EXTENDED_PUBLIC_KEY = 0x10,  /// extended public key of corresponding BIP32 path
    CMD_DERIVE_ADDRESS = 0x11,           /// derive address for corresponding BIP32 path
    CMD_ATTEST_INPUT_BOX = 0x20,         /// attest input box command
    CMD_SIGN_TRANSACTION = 0x21          /// sign transaction with BIP32 path
} command_e;

/**
 * Dispatch APDU command received to the right handler.
 *
 * @param[in] cmd
 *   Structured APDU command (CLA, INS, P1, P2, Lc, Command data).
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int apdu_dispatcher(const command_t *cmd);
