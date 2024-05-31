#pragma once

#include <stdint.h>  // uint*_t
#include "../../constants.h"
#include "ainpt_context.h"

/**
 * Send APDU response with Attested Input frame
 *
 * response =
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_attested_input_frame(attest_input_ctx_t *ctx,
                                       const uint8_t session_key[static SESSION_KEY_LEN],
                                       uint8_t index);

/**
 * Send APDU response with Attested Input frame count
 *
 * response = (uint8_t)frame_count
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_attested_input_frame_count(uint8_t tokens_count);

/**
 * Send APDU response with session_id
 *
 * response = (uint8_t)session_id
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_attested_input_session_id(uint8_t session_id);
