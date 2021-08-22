#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t

#include "ainpt_context.h"
#include "../../common/buffer.h"

typedef enum {
    ATTEST_INPUT_SUBCOMMAND_INIT = 0x01,
    ATTEST_INPUT_SUBCOMMAND_PREFIX_CHUNK = 0x02,
    ATTEST_INPUT_SUBCOMMAND_TOKEN_IDS = 0x03,
    ATTEST_INPUT_SUBCOMMAND_SUFFIX_CHUNK = 0x04,
    ATTEST_INPUT_SUBCOMMAND_BOX = 0x05,
    ATTEST_INPUT_SUBCOMMAND_BOX_TREE_CHUNK = 0x06,
    ATTEST_INPUT_SUBCOMMAND_BOX_TOKENS = 0x07,
    ATTEST_INPUT_SUBCOMMAND_BOX_REGISTER = 0x08,
    ATTEST_INPUT_SUBCOMMAND_GET_RESPONSE_FRAME = 0x09
} attest_input_subcommand_e;

/**
 * Handler for CMD_ATTEST_INPUT command.
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
int handler_attest_input(buffer_t *cdata,
                         attest_input_subcommand_e subcommand,
                         uint8_t session_or_token);
