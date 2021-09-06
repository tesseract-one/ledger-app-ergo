#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * Display application access_token on the device and ask confirmation to proceed.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_sign_tx_access_token(uint32_t app_access_token);

/**
 * Display transaction info on the device and ask confirmation to proceed.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_sign_tx_transaction(void);

/**
 * Action for sign tx input approval.
 *
 * @param[in] choice
 *   User choice (either approved or rejected).
 *
 */
void ui_action_sign_tx_token(bool approved);

/**
 * Action for transaction confirmation.
 *
 * @param[in] choice
 *   User choice (either approved or rejected).
 *
 */
void ui_action_sign_tx_transaction(bool approved);