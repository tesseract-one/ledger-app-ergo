#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t

#include "stx_context.h"
#include "../../common/buffer.h"

typedef enum {
    SIGN_TRANSACTION_SUBCOMMAND_INIT = 0x01,
    SIGN_TRANSACTION_SUBCOMMAND_TOKEN_IDS = 0x02,
    SIGN_TRANSACTION_SUBCOMMAND_INPUT_FRAME = 0x03,
    SIGN_TRANSACTION_SUBCOMMAND_INPUT_CONTEXT_EXTENSION = 0x04,
    SIGN_TRANSACTION_SUBCOMMAND_DATA_INPUT = 0x05,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT = 0x06,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_TREE_CHUNK = 0x07,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_MINERS_FEE_TREE = 0x08,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_CHANGE_TREE = 0x09,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_TOKENS = 0x0A,
    SIGN_TRANSACTION_SUBCOMMAND_OUTPUT_REGISTER = 0x0B,
    SIGN_TRANSACTION_SUBCOMMAND_CONFIRM = 0x0C,
    SIGN_TRANSACTION_SUBCOMMAND_SIGN_PK = 0x0D
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
