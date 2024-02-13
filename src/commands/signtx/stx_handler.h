#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t

#include <buffer.h>

#include "stx_context.h"

typedef enum {
    SIGN_TRANSACTION_SUBCOMMAND_SIGN_PK = 0x01,
    SIGN_TRANSACTION_SUBCOMMAND_START_TX = 0x10,
    SIGN_TRANSACTION_SUBCOMMAND_TOKEN_IDS = 0x11,
    SIGN_TRANSACTION_SUBCOMMAND_INPUT_FRAME = 0x12,
    SIGN_TRANSACTION_SUBCOMMAND_INPUT_CONTEXT_EXTENSION = 0x13,
    SIGN_TRANSACTION_SUBCOMMAND_DATA_INPUT = 0x14,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT = 0x15,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_TREE_CHUNK = 0x16,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_MINERS_FEE_TREE = 0x17,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_CHANGE_TREE = 0x18,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_TOKENS = 0x19,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_REGISTERS = 0x1A,
    SIGN_TRANSACTION_SUBCOMMAND_CONFIRM = 0x20
} sign_transaction_subcommand_e;

/**
 * Handler for CMD_ATTEST_SIGN_TX command.
 *
 * @param[in,out] cdata
 *   Command data. Check APDU documentation.
 * @param[in]     subcommand
 *   Subcommand identifier.
 * @param[in]     session_or_token
 *   Whether data has access token or not, or session id (depends on subcommand)
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_sign_transaction(buffer_t *cdata,
                             sign_transaction_subcommand_e subcommand,
                             uint8_t session_or_token);
