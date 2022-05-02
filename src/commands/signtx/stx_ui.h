#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../../ui/ui_approve_reject.h"
#include "stx_amounts.h"
#include "stx_types.h"

/**
 * Display application access_token on the device and ask confirmation to proceed.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_stx_add_display_access_token_screens(uint32_t app_access_token,
                                            uint8_t* screen,
                                            sign_transaction_ui_aprove_ctx_t* ctx);

/**
 * Display transaction info on the device and ask confirmation to proceed.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_stx_display_transaction_screens(sign_transaction_ui_confirm_ctx_t* ctx,
                                       const sign_transaction_amounts_ctx_t* amounts,
                                       uint8_t op_screen_count,
                                       ui_sign_transaction_operation_show_screen_cb screen_cb,
                                       ui_sign_transaction_operation_send_response_cb response_cb,
                                       void* cb_context);
