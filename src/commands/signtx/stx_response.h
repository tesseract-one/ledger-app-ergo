#pragma once

#include <stdint.h>  // uint*_t
#include "../../constants.h"
#include "stx_context.h"

/**
 * Send APDU response with session_id
 *
 * response = (uint8_t)session_id
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_sign_transaction_session_id(uint8_t session_id);
