#include <os.h>
#include <ux.h>

#include "stx_ui.h"
#include "stx_response.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../common/macros.h"
#include "../../helpers/response.h"
#include "../../common/int_ops.h"
#include "../../common/base58.h"
#include "../../common/format.h"
#include "../../ergo/address.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_dynamic_flow.h"
#include "../../ui/ui_menu.h"

#define ERGO_ID_UI_CHARACTERS_HALF 7

#define STRING_ADD_STATIC_TEXT(str, slen, text) \
    strncpy(str, text, slen);                   \
    slen -= sizeof(text) - 1;                   \
    str += sizeof(text) - 1

static inline void id_string_remove_middle(char* str, size_t len) {
    str[ERGO_ID_UI_CHARACTERS_HALF] = str[ERGO_ID_UI_CHARACTERS_HALF + 1] =
        str[ERGO_ID_UI_CHARACTERS_HALF + 2] = '.';
    memmove(str + ERGO_ID_UI_CHARACTERS_HALF + 3,
            str + len - ERGO_ID_UI_CHARACTERS_HALF,
            ERGO_ID_UI_CHARACTERS_HALF);
    str[2 * ERGO_ID_UI_CHARACTERS_HALF + 3] = '\0';
}

static inline bool format_hex_id(const uint8_t* id, size_t id_len, char* out, size_t out_len) {
    int len = format_hex(id, id_len, out, out_len);
    if (len <= 0) return false;
    id_string_remove_middle(out, len);
    return true;
}

static inline bool format_b58_id(const uint8_t* id, size_t id_len, char* out, size_t out_len) {
    int len = base58_encode(id, id_len, out, out_len);
    if (len <= 0) return false;
    id_string_remove_middle(out, len);
    return true;
}

// ----- OPERATION APPROVE / REJECT FLOW

static NOINLINE void ui_stx_operation_approve_action(bool approved, void* context) {
    sign_transaction_ui_aprove_ctx_t* ctx = (sign_transaction_ui_aprove_ctx_t*) context;
    sign_transaction_ctx_t* sign_tx = (sign_transaction_ctx_t*) ctx->sign_tx_context;

    G_context.is_ui_busy = false;

    if (approved) {
        G_context.app_session_id = ctx->app_token_value;
        sign_tx->state = SIGN_TRANSACTION_STATE_APPROVED;
        send_response_sign_transaction_session_id(sign_tx->session);
    } else {
        res_deny();
    }

    ui_menu_main();
}

bool ui_stx_add_operation_approve_screens(sign_transaction_ui_aprove_ctx_t* ctx,
                                          uint8_t* screen,
                                          uint32_t app_access_token,
                                          bool is_known_application,
                                          sign_transaction_ctx_t* sign_tx) {
    if (MAX_NUMBER_OF_SCREENS - *screen < 3) return false;

    if (!is_known_application) {
        G_ux_flow[(*screen)++] = ui_application_id_screen(app_access_token, ctx->app_token);
    }
    ctx->app_token_value = app_access_token;
    ctx->sign_tx_context = sign_tx;
    ctx->is_known_application = is_known_application;

    const ux_flow_step_t** approve = &G_ux_flow[(*screen)++];
    const ux_flow_step_t** reject = &G_ux_flow[(*screen)++];
    ui_approve_reject_screens(ui_stx_operation_approve_action, ctx, approve, reject);

    return true;
}

// --- OUTPUT APPROVE / REJECT FLOW

//  Fist flow step with icon and text
UX_STEP_NOCB(ux_stx_display_output_confirm_step, pn, {&C_icon_warning, "Confirm Output"});

static inline uint16_t output_info_print_address(const sign_transaction_output_info_ctx_t* ctx,
                                                 uint8_t network_id,
                                                 char* title,
                                                 uint8_t title_len,
                                                 char* address,
                                                 uint8_t address_len) {
    if (!stx_output_info_is_finished(ctx)) return SW_BAD_STATE;
    switch (stx_output_info_type(ctx)) {
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32: {
            strncpy(title, "Change", title_len);
            if (!bip32_path_format(ctx->bip32_path.path,
                                   ctx->bip32_path.len,
                                   address,
                                   address_len)) {
                return SW_BIP32_FORMATTING_FAILED;
            }
            break;
        }
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_ADDRESS: {
            uint8_t raw_address[P2PK_ADDRESS_LEN];
            strncpy(title, "Address", title_len);
            if (!ergo_address_from_compressed_pubkey(network_id, ctx->public_key, raw_address)) {
                return SW_ADDRESS_GENERATION_FAILED;
            }
            if (!format_b58_id(raw_address, P2PK_ADDRESS_LEN, address, address_len)) {
                return SW_ADDRESS_FORMATTING_FAILED;
            }
            break;
        }
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT:
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_SCRIPT_HASH: {
            strncpy(title, "Script Hash", title_len);
            uint8_t raw_address[P2SH_ADDRESS_LEN];
            if (!ergo_address_from_script_hash(network_id, ctx->tree_hash, raw_address)) {
                return SW_ADDRESS_GENERATION_FAILED;
            }
            if (!format_b58_id(raw_address, P2SH_ADDRESS_LEN, address, address_len)) {
                return SW_ADDRESS_FORMATTING_FAILED;
            }
            break;
        }
        case SIGN_TRANSACTION_OUTPUT_INFO_TYPE_MINERS_FEE: {
            strncpy(title, "Fee", title_len);
            strncpy(address, "Miners Fee", address_len);
            break;
        }
        default:
            return SW_BAD_STATE;
    }
    return SW_OK;
}

static NOINLINE uint16_t ui_stx_display_output_state(uint8_t screen,
                                                     char* title,
                                                     char* text,
                                                     void* context) {
    sign_transaction_ui_output_confirm_ctx_t* ctx =
        (sign_transaction_ui_output_confirm_ctx_t*) context;
    uint8_t title_len = MEMBER_SIZE(sign_transaction_ui_output_confirm_ctx_t, title);
    uint8_t text_len = MEMBER_SIZE(sign_transaction_ui_output_confirm_ctx_t, text);
    memset(title, 0, title_len);
    memset(text, 0, text_len);

    switch (screen) {
        case 0:  // Output Address Info
            return output_info_print_address(ctx->output,
                                             ctx->network_id,
                                             title,
                                             title_len,
                                             text,
                                             text_len);
        case 1: {  // Output Value
            strncpy(title, "Output Value", title_len);
            format_fpu64(text, text_len, ctx->output->value, ERGO_ERG_FRACTION_DIGIT_COUNT);
            break;
        }
        default: {        // Tokens
            screen -= 2;  // Decrease index for info screens
            uint8_t token_idx = stx_output_info_used_token_index(ctx->output, screen / 2);
            if (!IS_ELEMENT_FOUND(token_idx)) {  // error. bad index state
                return SW_BAD_TOKEN_INDEX;
            }
            if (screen % 2 == 0) {  // Token ID
                snprintf(title, title_len, "Token [%d]", (int) (screen / 2) + 1);
                if (!format_hex_id(ctx->output->tokens_table->tokens[token_idx],
                                   ERGO_ID_LEN,
                                   text,
                                   text_len)) {
                    return SW_ADDRESS_FORMATTING_FAILED;
                }
            } else {  // Token Value
                snprintf(title, title_len, "Token [%d] Value", (int) (screen / 2) + 1);
                format_u64(text, text_len, ctx->output->tokens[token_idx]);
            }
            break;
        }
    }
    return SW_OK;
}

static NOINLINE void ui_stx_operation_output_confirm_action(bool approved, void* context) {
    UNUSED(context);
    G_context.is_ui_busy = false;
    if (approved) {
        res_ok();
    } else {
        res_deny();
    }
    ui_menu_main();
}

bool ui_stx_add_output_screens(sign_transaction_ui_output_confirm_ctx_t* ctx,
                               uint8_t* screen,
                               const sign_transaction_output_info_ctx_t* output,
                               uint8_t network_id) {
    if (MAX_NUMBER_OF_SCREENS - *screen < 6) return false;

    memset(ctx, 0, sizeof(sign_transaction_ui_output_confirm_ctx_t));

    uint8_t tokens_count = stx_output_info_used_tokens_count(output);

    G_ux_flow[(*screen)++] = &ux_stx_display_output_confirm_step;

    if (!ui_add_dynamic_flow_screens(screen,
                                     2 + (2 * tokens_count),
                                     ctx->title,
                                     ctx->text,
                                     &ui_stx_display_output_state,
                                     (void*) ctx))
        return false;

    if (MAX_NUMBER_OF_SCREENS - *screen < 2) return false;

    const ux_flow_step_t** approve = &G_ux_flow[(*screen)++];
    const ux_flow_step_t** reject = &G_ux_flow[(*screen)++];
    ui_approve_reject_screens(ui_stx_operation_output_confirm_action, NULL, approve, reject);

    ctx->network_id = network_id;
    ctx->output = output;

    return true;
}

// --- TX ACCEPT / REJECT FLOW

// Fist flow step with icon and text
UX_STEP_NOCB(ux_stx_display_sign_confirm_step, pn, {&C_icon_warning, "Approve Signing"});

// Callback for TX UI rendering
static NOINLINE uint16_t ui_stx_display_tx_state(uint8_t screen,
                                                 char* title,
                                                 char* text,
                                                 void* context) {
    sign_transaction_ui_sign_confirm_ctx_t* ctx = (sign_transaction_ui_sign_confirm_ctx_t*) context;
    uint8_t title_len = MEMBER_SIZE(sign_transaction_ui_sign_confirm_ctx_t, title);
    uint8_t text_len = MEMBER_SIZE(sign_transaction_ui_sign_confirm_ctx_t, text);
    memset(title, 0, title_len);
    memset(text, 0, text_len);

    if (screen < ctx->op_screen_count) {  // Showing operation screen
        return ctx->op_screen_cb(screen, title, title_len, text, text_len, ctx->op_cb_context);
    }
    screen -= ctx->op_screen_count;
    switch (screen) {
        case 0: {  // TX Value
            strncpy(title, "Transaction Amount", title_len);
            format_fpu64(text, text_len, ctx->amounts->value, ERGO_ERG_FRACTION_DIGIT_COUNT);
            break;
        }
        case 1: {  // TX Fee
            strncpy(title, "Transaction Fee", title_len);
            format_fpu64(text, text_len, ctx->amounts->fee, ERGO_ERG_FRACTION_DIGIT_COUNT);
            break;
        }
        default: {        // Tokens
            screen -= 2;  // Decrease index for info screens
            uint8_t token_idx = stx_amounts_non_zero_token_index(ctx->amounts, screen / 2);
            if (!IS_ELEMENT_FOUND(token_idx)) {  // error. bad index state
                return SW_BAD_TOKEN_INDEX;
            }
            if (screen % 2 == 0) {  // Token ID
                snprintf(title, title_len, "Token [%d]", (int) (screen / 2) + 1);
                if (!format_hex_id(ctx->amounts->tokens_table.tokens[token_idx],
                                   ERGO_ID_LEN,
                                   text,
                                   text_len)) {
                    return SW_ADDRESS_FORMATTING_FAILED;
                }
            } else {  // Token Value
                snprintf(title, title_len, "Token [%d] Value", (int) (screen / 2) + 1);
                int64_t value = ctx->amounts->tokens[token_idx];
                if (value < 0) {  // output > inputs
                    STRING_ADD_STATIC_TEXT(text, text_len, "Minting: ");
                    format_u64(text, text_len, -value);
                } else {  // inputs > outputs
                    STRING_ADD_STATIC_TEXT(text, text_len, "Burning: ");
                    format_u64(text, text_len, value);
                }
            }
            break;
        }
    }
    return SW_OK;
}

// TX approve/reject callback
static NOINLINE void ui_stx_operation_execute_action(bool approved, void* context) {
    G_context.is_ui_busy = false;
    sign_transaction_ui_sign_confirm_ctx_t* ctx = (sign_transaction_ui_sign_confirm_ctx_t*) context;
    if (approved) {
        ctx->op_response_cb(ctx->op_cb_context);
    } else {
        res_deny();
    }
    clear_context(&G_context, CMD_NONE);
    ui_menu_main();
}

bool ui_stx_add_transaction_screens(sign_transaction_ui_sign_confirm_ctx_t* ctx,
                                    uint8_t* screen,
                                    const sign_transaction_amounts_ctx_t* amounts,
                                    uint8_t op_screen_count,
                                    ui_sign_transaction_operation_show_screen_cb screen_cb,
                                    ui_sign_transaction_operation_send_response_cb response_cb,
                                    void* cb_context) {
    if (MAX_NUMBER_OF_SCREENS - *screen < 6) return false;

    memset(ctx, 0, sizeof(sign_transaction_ui_sign_confirm_ctx_t));

    uint8_t tokens_count = stx_amounts_non_zero_tokens_count(amounts);

    G_ux_flow[(*screen)++] = &ux_stx_display_sign_confirm_step;

    if (!ui_add_dynamic_flow_screens(screen,
                                     op_screen_count + 2 + (2 * tokens_count),
                                     ctx->title,
                                     ctx->text,
                                     &ui_stx_display_tx_state,
                                     (void*) ctx))
        return false;

    if (MAX_NUMBER_OF_SCREENS - *screen < 2) return false;

    const ux_flow_step_t** approve = &G_ux_flow[(*screen)++];
    const ux_flow_step_t** reject = &G_ux_flow[(*screen)++];
    ui_approve_reject_screens(ui_stx_operation_execute_action, ctx, approve, reject);

    ctx->op_screen_count = op_screen_count;
    ctx->op_screen_cb = screen_cb;
    ctx->op_response_cb = response_cb;
    ctx->op_cb_context = cb_context;
    ctx->amounts = amounts;

    return true;
}

bool ui_stx_display_screens(uint8_t screen_count) {
    if (MAX_NUMBER_OF_SCREENS - screen_count < 2) return false;

    G_ux_flow[screen_count++] = FLOW_LOOP;
    G_ux_flow[screen_count++] = FLOW_END_STEP;

    ux_flow_init(0, G_ux_flow, NULL);

    G_context.is_ui_busy = true;

    return true;
}
