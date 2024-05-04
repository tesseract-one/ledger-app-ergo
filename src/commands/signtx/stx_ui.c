#include <os.h>
#include <ux.h>
#include <glyphs.h>
#include <format.h>
#include <base58.h>

#include "stx_ui.h"
#include "stx_response.h"

#include "../../context.h"
#include "../../common/macros_ext.h"
#include "../../helpers/response.h"
#include "../../common/safeint.h"
#include "../../ergo/address.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_dynamic_flow.h"
#include "../../ui/ui_menu.h"
#include "../../ui/ui_main.h"

#ifdef TARGET_NANOS
#define ERGO_ID_UI_CHARACTERS_HALF 7
#else
#define ERGO_ID_UI_CHARACTERS_HALF 26
#endif

#define STRING_ADD_STATIC_TEXT(str, slen, text) \
    strncpy(str, text, slen);                   \
    slen -= sizeof(text) - 1;                   \
    str += sizeof(text) - 1

static inline void id_string_remove_middle(char* str, size_t len) {
    if (len <= 2 * ERGO_ID_UI_CHARACTERS_HALF + 3) return;
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
    id_string_remove_middle(out, len - 1);
    return true;
}

static inline bool format_b58_id(const uint8_t* id, size_t id_len, char* out, size_t out_len) {
    int len = base58_encode(id, id_len, out, out_len);
    if (len <= 0) return false;
    return true;
}

static inline bool format_erg_amount(uint64_t amount, char* out, size_t out_len) {
    if (!format_fpu64(out, out_len, amount, ERGO_ERG_FRACTION_DIGIT_COUNT)) {
        return false;
    }
    size_t out_bytes = strlen(out);
    if (out_len < 5 || out_bytes > out_len - 5) return false;
    out_len -= out_bytes;
    out += out_bytes;
    STRING_ADD_STATIC_TEXT(out, out_len, " ERG");
    return true;
}

// ----- OPERATION APPROVE / REJECT FLOW

void ui_stx_operation_approve_reject(bool approved, sign_transaction_ui_aprove_ctx_t* ctx) {
    sign_transaction_ctx_t* sign_tx = (sign_transaction_ctx_t*) ctx->sign_tx_context;

    app_set_ui_busy(false);

    if (approved) {
        app_set_connected_app_id(ctx->app_token_value);
        sign_tx->state = SIGN_TRANSACTION_STATE_APPROVED;
        send_response_sign_transaction_session_id(sign_tx->session);
    } else {
        app_set_current_command(CMD_NONE);
        res_deny();
    }

    ui_menu_main();
}

static NOINLINE void ui_stx_operation_approve_action(bool approved, void* context) {
    sign_transaction_ui_aprove_ctx_t* ctx = (sign_transaction_ui_aprove_ctx_t*) context;
    ui_stx_operation_approve_reject(approved, ctx);
}

bool ui_stx_add_operation_approve_screens(sign_transaction_ui_aprove_ctx_t* ctx,
                                          uint8_t* screen,
                                          uint32_t app_access_token,
                                          bool is_known_application,
                                          sign_transaction_ctx_t* sign_tx) {
    if (MAX_NUMBER_OF_SCREENS - *screen < 3) return false;

    if (!is_known_application) {
        ui_add_screen(ui_application_id_screen(app_access_token, ctx->app_token), screen);
    }
    ctx->app_token_value = app_access_token;
    ctx->sign_tx_context = sign_tx;
    ctx->is_known_application = is_known_application;

    ui_approve_reject_screens(ui_stx_operation_approve_action,
                              ctx,
                              ui_next_sreen_ptr(screen),
                              ui_next_sreen_ptr(screen));

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
            if (!format_erg_amount(ctx->output->value, text, text_len)) {
                return SW_BUFFER_ERROR;
            }
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
    sign_transaction_ui_output_confirm_ctx_t* ctx =
        (sign_transaction_ui_output_confirm_ctx_t*) context;
    app_set_ui_busy(false);

    explicit_bzero(ctx->last_approved_change, sizeof(sign_transaction_bip32_path_t));

    if (approved) {
        // store last approved change address
        if (stx_output_info_type(ctx->output) == SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32) {
            memmove(ctx->last_approved_change,
                    &ctx->output->bip32_path,
                    sizeof(sign_transaction_bip32_path_t));
        }
        res_ok();
    } else {
        app_set_current_command(CMD_NONE);
        res_deny();
    }

    ui_menu_main();
}

bool ui_stx_add_output_screens(sign_transaction_ui_output_confirm_ctx_t* ctx,
                               uint8_t* screen,
                               const sign_transaction_output_info_ctx_t* output,
                               sign_transaction_bip32_path_t* last_approved_change,
                               uint8_t network_id) {
    if (MAX_NUMBER_OF_SCREENS - *screen < 6) return false;

    memset(ctx, 0, sizeof(sign_transaction_ui_output_confirm_ctx_t));
    memset(last_approved_change, 0, sizeof(sign_transaction_bip32_path_t));

    ui_add_screen(&ux_stx_display_output_confirm_step, screen);

    uint8_t info_screen_count = 1;  // Address screen
    if (stx_output_info_type(output) != SIGN_TRANSACTION_OUTPUT_INFO_TYPE_BIP32) {
        uint8_t tokens_count = stx_output_info_used_tokens_count(output);
        info_screen_count += 1 + (2 * tokens_count);  // value screen + tokens (2 for each)
    }

    if (!ui_add_dynamic_flow_screens(screen,
                                     info_screen_count,
                                     ctx->title,
                                     ctx->text,
                                     &ui_stx_display_output_state,
                                     (void*) ctx))
        return false;

    if (MAX_NUMBER_OF_SCREENS - *screen < 2) return false;

    ui_approve_reject_screens(ui_stx_operation_output_confirm_action,
                              (void*) ctx,
                              ui_next_sreen_ptr(screen),
                              ui_next_sreen_ptr(screen));

    ctx->network_id = network_id;
    ctx->output = output;
    ctx->last_approved_change = last_approved_change;

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
            if (!format_erg_amount(ctx->amounts->value, text, text_len)) {
                return SW_BUFFER_ERROR;
            }
            break;
        }
        case 1: {  // TX Fee
            strncpy(title, "Transaction Fee", title_len);
            if (!format_erg_amount(ctx->amounts->fee, text, text_len)) {
                return SW_BUFFER_ERROR;
            }
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
    app_set_ui_busy(false);

    sign_transaction_ui_sign_confirm_ctx_t* ctx = (sign_transaction_ui_sign_confirm_ctx_t*) context;
    if (approved) {
        ctx->op_response_cb(ctx->op_cb_context);
    } else {
        res_deny();
    }

    app_set_current_command(CMD_NONE);
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

    ui_add_screen(&ux_stx_display_sign_confirm_step, screen);

    if (!ui_add_dynamic_flow_screens(screen,
                                     op_screen_count + 2 + (2 * tokens_count),
                                     ctx->title,
                                     ctx->text,
                                     &ui_stx_display_tx_state,
                                     (void*) ctx))
        return false;

    if (MAX_NUMBER_OF_SCREENS - *screen < 2) return false;

    ui_approve_reject_screens(ui_stx_operation_execute_action,
                              ctx,
                              ui_next_sreen_ptr(screen),
                              ui_next_sreen_ptr(screen));

    ctx->op_screen_count = op_screen_count;
    ctx->op_screen_cb = screen_cb;
    ctx->op_response_cb = response_cb;
    ctx->op_cb_context = cb_context;
    ctx->amounts = amounts;

    return true;
}

bool ui_stx_display_screens(uint8_t screen_count) {
    return ui_display_screens(&screen_count);
}
