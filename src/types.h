#pragma once

#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t

#include "constants.h"
#include "common/bip32.h"

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
    CMD_NONE = 0x00,                    /// empty command
    CMD_GET_APP_VERSION = 0x01,         /// version of the application
    CMD_GET_APP_NAME = 0x02,            /// application name
    CMD_GET_EXTENDED_PUBLIC_KEY = 0x10, /// extended public key of corresponding BIP32 path
    CMD_DERIVE_ADDRESS = 0x11,          /// derive address for corresponding BIP32 path
    CMD_ATTEST_INPUT_BOX = 0x20,        /// attest input box command
    CMD_SIGN_TRANSACTION = 0x21         /// sign transaction with BIP32 path
} command_e;

/**
 * Structure with fields of APDU command.
 */
typedef struct {
    uint8_t cla;    /// Instruction class
    command_e ins;  /// Instruction code
    uint8_t p1;     /// Instruction parameter 1
    uint8_t p2;     /// Instruction parameter 2
    uint8_t lc;     /// Lenght of command data
    uint8_t *data;  /// Command data
} command_t;

// /**
//  * Structure for public key context information.
//  */
// typedef struct {
//     uint8_t public_key[32];     /// raw public key
//     uint8_t chain_code[32];     /// chain code for public key derivation
// } extended_pubkey_t;

// /**
//  * Structure for transaction information context.
//  */
// typedef struct {
//     uint8_t raw_tx[MAX_TRANSACTION_LEN];  /// raw transaction serialized
//     size_t raw_tx_len;                    /// length of raw transaction
//     transaction_t transaction;            /// structured transaction
//     uint8_t m_hash[32];                   /// message hash digest
//     uint8_t signature[MAX_DER_SIG_LEN];   /// transaction signature encoded in DER
//     uint8_t signature_len;                /// length of transaction signature
//     uint8_t v;                            /// parity of y-coordinate of R in ECDSA signature
// } transaction_ctx_t;

typedef enum {
    CMD_SIGN_TRANSACTION_NONE = 0x00,
    CMD_SIGN_TRANSACTION_START = 0x01,
    CMD_SIGN_TRANSACTION_ADD_TOKEN_IDS = 0x02,
    CMD_SIGN_TRANSACTION_ADD_INPUT_BOX = 0x03,
    CMD_SIGN_TRANSACTION_ADD_DATA_INPUTS = 0x04,
    CMD_SIGN_TRANSACTION_ADD_OUTPUT_BOX = 0x05,
    CMD_SIGN_TRANSACTION_CONFIRM = 0x06,
    CMD_SIGN_TRANSACTION_SIGN_TX_HASH_WITH_KEY = 0x07
} sign_transaction_step_e;

typedef struct {
    uint8_t session_id;
    sign_transaction_step_e step;
    uint16_t inputs_count;
    uint16_t data_inputs_count;
    uint8_t distinct_token_ids_count;
    uint16_t output_count;
    uint64_t amount;
    // token_amount_table_t tokens;
    bool approved;
    uint8_t tx_hash[TRANSACTION_HASH_LEN];
} sign_transaction_ctx_t;
