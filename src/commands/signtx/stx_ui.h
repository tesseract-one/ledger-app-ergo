#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "../../ui/ui_approve_reject.h"
#include "stx_amounts.h"
#include "stx_types.h"
#include "stx_context.h"

/**
 * Add application access_token and accept/reject screens to the UI.
 *
 * @return true if success, false if screens flow is full.
 *
 */
bool ui_stx_add_operation_approve_screens(sign_transaction_ui_aprove_ctx_t* ctx,
                                          uint8_t* screen,
                                          uint32_t app_access_token,
                                          bool is_known_application,
                                          sign_transaction_ctx_t* sign_tx);

/**
 * Add output info and accept/reject screens to the UI.
 *
 * @return true if success, false if screens flow is full.
 *
 */
bool ui_stx_add_output_screens(sign_transaction_ui_output_confirm_ctx_t* ctx,
                               uint8_t* screen,
                               const sign_transaction_output_info_ctx_t* output,
                               sign_transaction_bip32_path_t* last_approved_change,
                               uint8_t network_id);

/**
 * Add transaction info and accept/reject screens to the UI.
 *
 * @return true if success, false if screens flow is full.
 *
 */
bool ui_stx_add_transaction_screens(sign_transaction_ui_sign_confirm_ctx_t* ctx,
                                    uint8_t* screen,
                                    const sign_transaction_amounts_ctx_t* amounts,
                                    uint8_t op_screen_count,
                                    ui_sign_transaction_operation_show_screen_cb screen_cb,
                                    ui_sign_transaction_operation_send_response_cb response_cb,
                                    void* cb_context);

/**
 * Finalizes screen flow and pushes it to the screen.
 *
 * @return true if success, false if screens buffer is full.
 *
 */
bool ui_stx_display_screens(uint8_t screen_count);

/**
 * Approve or reject operation programmatically.
 *
 */
void ui_stx_operation_approve_reject(bool approved, sign_transaction_ui_aprove_ctx_t* ctx);