#pragma once

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t

#include "constants.h"

/**
 * Enumeration for the status of IO.
 */
typedef enum {
    READY,     /// ready for new event
    RECEIVED,  /// data received
    WAITING    /// waiting
} io_state_e;

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
 * Structure with fields of APDU command.
 */
typedef struct {
    uint8_t cla;    /// Instruction class
    command_e ins;  /// Instruction code
    uint8_t p1;     /// Instruction parameter 1
    uint8_t p2;     /// Instruction parameter 2
    uint8_t lc;     /// Length of command data
    uint8_t *data;  /// Command data
} command_t;
