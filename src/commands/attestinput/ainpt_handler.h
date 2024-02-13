#pragma once

#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include <stdint.h>   // uint*_t

#include <buffer.h>

#include "ainpt_context.h"

typedef enum {
    ATTEST_INPUT_SUBCOMMAND_INIT = 0x01,
    ATTEST_INPUT_SUBCOMMAND_TREE_CHUNK = 0x02,
    ATTEST_INPUT_SUBCOMMAND_TOKENS = 0x03,
    ATTEST_INPUT_SUBCOMMAND_REGISTERS = 0x04,
    ATTEST_INPUT_SUBCOMMAND_GET_RESPONSE_FRAME = 0x05
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
 * @param[in]     app_context
 *   Whether data has access token or not, or session id (depends on subcommand)
 *
 * @return zero or positive integer if success, negative integer otherwise.
 *
 */
int handler_attest_input(buffer_t *cdata,
                         attest_input_subcommand_e subcommand,
                         uint8_t session_or_token);
