//
//  autxo_response.h
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#pragma once

#include <stdint.h>   // uint*_t

/**
 * Send APDU response with Attested Input frame
 *
 * response =
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_attested_input_frame(uint8_t index);

/**
 * Send APDU response with Attested Input frame count
 *
 * response = (uint8_t)frame_count
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_attested_input_frame_count(void);


/**
 * Send APDU response with session_id
 *
 * response = (uint8_t)session_id
 *
 * @return zero or positive integer if success, -1 otherwise.
 *
 */
int send_response_attested_input_session_id(void);
