#pragma once

#include <stdint.h>   // uint*
#include <stdbool.h>  // bool

// /**
//  * Callback to reuse action with approve/reject in step FLOW.
//  */
// typedef void (*action_validate_cb)(bool);

/**
 * Display account on the device and ask confirmation to export.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_address(bool send, uint8_t network_id, uint32_t app_access_token);

/**
 * Action for public key validation and export.
 *
 * @param[in] choice
 *   User choice (either approved or rejected).
 *
 */
void ui_action_derive_address(bool choice);