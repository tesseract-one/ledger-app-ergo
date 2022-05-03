#include <os.h>
#include <ux.h>

#include "stx_ui.h"
#include "stx_response.h"
#include "stx_sw.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../common/macros.h"
#include "../../helpers/response.h"
#include "../../common/int_ops.h"
#include "../../common/base58.h"
#include "../../common/format.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_menu.h"

#define CONTEXT(gctx) gctx.ctx.sign_tx

#define STRING_ADD_STATIC_TEXT(str, slen, text) \
    strncpy(str, text, slen);                   \
    slen -= sizeof(text) - 1;                   \
    str += sizeof(text) - 1

#define DISPLAY_TX_STATE(ctx, switch_method)      \
    do {                                          \
        uint16_t res = ui_stx_display_state(ctx); \
        if (res == SW_OK) {                       \
            switch_method();                      \
        } else {                                  \
            res_error(res);                       \
            clear_context(&G_context, CMD_NONE);  \
            ui_menu_main();                       \
        }                                         \
    } while (0)

#define TOKEN_ID_CHARACTERS 7

static NOINLINE void ui_stx_operation_approve_action(bool approved) {
    sign_transaction_ui_aprove_ctx_t* ctx =
        (sign_transaction_ui_aprove_ctx_t*) CONTEXT(G_context).ui_context;

    G_context.is_ui_busy = false;

    if (approved) {
        G_context.app_session_id = ctx->app_token_value;
        CONTEXT(G_context).state = SIGN_TRANSACTION_STATE_APPROVED;
        send_response_sign_transaction_session_id(CONTEXT(G_context).session);
    } else {
        res_deny();
    }

    ui_menu_main();
}

bool ui_stx_add_access_token_screens(uint32_t app_access_token,
                                     uint8_t* screen,
                                     sign_transaction_ui_aprove_ctx_t* ctx) {
    if (MAX_NUMBER_OF_SCREENS - *screen < 3) return false;

    if (app_access_token != 0) {
        G_ux_flow[(*screen)++] = ui_application_id_screen(app_access_token, ctx->app_token);
    }
    ctx->app_token_value = app_access_token;

    const ux_flow_step_t** approve = &G_ux_flow[(*screen)++];
    const ux_flow_step_t** reject = &G_ux_flow[(*screen)++];
    ui_approve_reject_screens(ui_stx_operation_approve_action, approve, reject);

    return true;
}

// This is a special function we must call for bnnn_paging to work properly in an edgecase.
// It does some weird stuff with the `G_ux` global which is defined by the SDK.
// No need to dig deeper into the code, a simple copy paste will do.
void bnnn_paging_edgecase() {
    G_ux.flow_stack[G_ux.stack_count - 1].prev_index =
        G_ux.flow_stack[G_ux.stack_count - 1].index - 2;
    G_ux.flow_stack[G_ux.stack_count - 1].index--;
    ux_flow_relayout();
}

static NOINLINE uint16_t ui_stx_display_state(sign_transaction_ui_confirm_ctx_t* ctx) {
    char* title = ctx->title;
    char* text = ctx->text;
    uint8_t title_len = MEMBER_SIZE(sign_transaction_ui_confirm_ctx_t, title);
    uint8_t text_len = MEMBER_SIZE(sign_transaction_ui_confirm_ctx_t, text);
    memset(title, 0, title_len);
    memset(text, 0, text_len);
    switch (ctx->state) {
        case SIGN_TRANSACTION_UI_STATE_OPERATION_SCREEN: {
            return ctx->op_screen_cb(ctx->op_screen_index,
                                     title,
                                     title_len,
                                     text,
                                     text_len,
                                     ctx->op_cb_context);
        }
        case SIGN_TRANSACTION_UI_STATE_TX_VALUE: {
            strncpy(title, "Transaction Amount", title_len);
            uint64_t value = ctx->amounts->inputs;
            if (!checked_sub_u64(value, ctx->amounts->fee, &value) ||
                !checked_sub_u64(value, ctx->amounts->change, &value)) {
                strncpy(text, "Bad TX. Outputs is bigger than inputs", text_len);
            } else {
                format_fpu64(text, text_len, value, ERGO_ERG_FRACTION_DIGIT_COUNT);
            }
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TX_FEE: {
            strncpy(title, "Transaction Fee", title_len);
            format_fpu64(text, text_len, ctx->amounts->fee, ERGO_ERG_FRACTION_DIGIT_COUNT);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TOKEN_ID: {
            uint8_t token_idx = ctx->token_idx;
            snprintf(title, title_len, "Token [%d]", (int) token_idx + 1);
            int len = base58_encode(ctx->amounts->tokens_table.tokens[token_idx],
                                    ERGO_ID_LEN,
                                    text,
                                    text_len);
            text[TOKEN_ID_CHARACTERS] = text[TOKEN_ID_CHARACTERS + 1] =
                text[TOKEN_ID_CHARACTERS + 2] = '.';
            memmove(text + TOKEN_ID_CHARACTERS + 3,
                    text + len - TOKEN_ID_CHARACTERS,
                    TOKEN_ID_CHARACTERS);
            text[2 * TOKEN_ID_CHARACTERS + 3] = '\0';
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE: {
            uint8_t token_idx = ctx->token_idx;
            snprintf(title, title_len, "Token [%d]", (int) token_idx + 1);
            const sign_transaction_token_amount_t* amount = &ctx->amounts->tokens[token_idx];
            uint64_t value = 0;
            bool minting;
            if (!checked_sub_u64(amount->input, amount->output, &value)) {
                minting = true;
                checked_sub_u64(amount->output, amount->input, &value);
                checked_add_u64(value, amount->change, &value);
            } else if (amount->change > value) {
                minting = true;
                checked_sub_u64(amount->change, value, &value);
            } else {
                minting = false;
                checked_sub_u64(value, amount->change, &value);
            }
            if (minting || value != 0) {  // Minting or Burning
                if (minting) {
                    STRING_ADD_STATIC_TEXT(text, text_len, "M: ");
                } else {
                    STRING_ADD_STATIC_TEXT(text, text_len, "B: ");
                }
                int char_count = format_u64(text, text_len, value);
                text_len -= char_count;
                text += char_count;
                STRING_ADD_STATIC_TEXT(text, text_len, "; ");
            }
            STRING_ADD_STATIC_TEXT(text, text_len, "T: ");
            format_u64(text, text_len, amount->output);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_NONE: {
            text[0] = title[0] = '\0';
            break;
        }
    }
    return SW_OK;
}

static inline void ui_stx_dynamic_step_right() {
    sign_transaction_ui_confirm_ctx_t* ctx =
        (sign_transaction_ui_confirm_ctx_t*) CONTEXT(G_context).ui_context;
    switch (ctx->state) {
        case SIGN_TRANSACTION_UI_STATE_NONE: {
            if (ctx->amounts->tokens_table.count > 0) {
                // TX has tokens. Show last token value
                ctx->state = SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE;
                ctx->token_idx = ctx->amounts->tokens_table.count - 1;
            } else {
                // TX doesn't have tokens. Show TX fee value
                ctx->state = SIGN_TRANSACTION_UI_STATE_TX_FEE;
                ctx->token_idx = 0;
            }
            // Fill screen with data
            //
            // `bnnn_paging_edgecase()` is similar to `ux_flow_prev()` but updates layout to account
            // for `bnnn_paging`'s weird behaviour.
            DISPLAY_TX_STATE(ctx, bnnn_paging_edgecase);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_OPERATION_SCREEN: {
            if (ctx->op_screen_index + 1 >= ctx->op_screen_count) {
                ctx->state = SIGN_TRANSACTION_UI_STATE_TX_VALUE;
            } else {
                ctx->state = SIGN_TRANSACTION_UI_STATE_OPERATION_SCREEN;
                ctx->op_screen_index++;
            }
            // Fill screen with data
            //
            // `bnnn_paging_edgecase()` is similar to `ux_flow_prev()` but updates layout to account
            // for `bnnn_paging`'s weird behaviour.
            DISPLAY_TX_STATE(ctx, bnnn_paging_edgecase);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TX_VALUE: {
            ctx->state = SIGN_TRANSACTION_UI_STATE_TX_FEE;
            // Fill screen with data
            //
            // `bnnn_paging_edgecase()` is similar to `ux_flow_prev()` but updates layout to account
            // for `bnnn_paging`'s weird behaviour.
            DISPLAY_TX_STATE(ctx, bnnn_paging_edgecase);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TX_FEE: {
            if (ctx->amounts->tokens_table.count > 0) {
                ctx->token_idx = 0;
                ctx->state = SIGN_TRANSACTION_UI_STATE_TOKEN_ID;
                // Fill screen with data
                //
                // `bnnn_paging_edgecase()` is similar to `ux_flow_prev()` but updates layout to
                // account for `bnnn_paging`'s weird behaviour.
                DISPLAY_TX_STATE(ctx, bnnn_paging_edgecase);
            } else {
                ctx->state = SIGN_TRANSACTION_UI_STATE_NONE;
                // go to the next static screen
                ux_flow_next();
            }
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TOKEN_ID: {
            ctx->state = SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE;
            // Fill screen with data
            //
            // `bnnn_paging_edgecase()` is similar to `ux_flow_prev()` but updates layout to account
            // for `bnnn_paging`'s weird behaviour.
            DISPLAY_TX_STATE(ctx, bnnn_paging_edgecase);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE: {
            if (ctx->token_idx < ctx->amounts->tokens_table.count - 1) {
                ctx->token_idx++;
                ctx->state = SIGN_TRANSACTION_UI_STATE_TOKEN_ID;
                // Fill screen with data
                //
                // `bnnn_paging_edgecase()` is similar to `ux_flow_prev()` but updates layout to
                // account for `bnnn_paging`'s weird behaviour.
                DISPLAY_TX_STATE(ctx, bnnn_paging_edgecase);
            } else {
                ctx->state = SIGN_TRANSACTION_UI_STATE_NONE;
                // go to the next static screen
                ux_flow_next();
            }
            break;
        }
    }
}

static inline void ui_stx_dynamic_step_left() {
    sign_transaction_ui_confirm_ctx_t* ctx =
        (sign_transaction_ui_confirm_ctx_t*) CONTEXT(G_context).ui_context;
    switch (ctx->state) {
        case SIGN_TRANSACTION_UI_STATE_NONE: {
            if (ctx->op_screen_count > 0) {
                // Show screen from operation hook
                ctx->state = SIGN_TRANSACTION_UI_STATE_OPERATION_SCREEN;
                ctx->op_screen_index = 0;
            } else {
                // Show TX ERG value
                ctx->state = SIGN_TRANSACTION_UI_STATE_TX_VALUE;
            }
            // Fill screen with data
            // Move to the next step, which will display the screen.
            DISPLAY_TX_STATE(ctx, ux_flow_next);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_OPERATION_SCREEN: {
            if (ctx->op_screen_index > 0) {
                ctx->op_screen_index--;
                ctx->state = SIGN_TRANSACTION_UI_STATE_OPERATION_SCREEN;
                // Fill screen with data
                // Move to the next step, which will display the screen.
                DISPLAY_TX_STATE(ctx, ux_flow_next);
            } else {
                ctx->state = SIGN_TRANSACTION_UI_STATE_NONE;
                // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s
                // weird behaviour.
                bnnn_paging_edgecase();
            }
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TX_VALUE: {
            if (ctx->op_screen_count > 0) {
                ctx->op_screen_index = ctx->op_screen_count - 1;
                ctx->state = SIGN_TRANSACTION_UI_STATE_OPERATION_SCREEN;
                // Fill screen with data
                // Move to the next step, which will display the screen.
                DISPLAY_TX_STATE(ctx, ux_flow_next);
            } else {
                ctx->state = SIGN_TRANSACTION_UI_STATE_NONE;
                // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s
                // weird behaviour.
                bnnn_paging_edgecase();
            }
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TX_FEE: {
            ctx->state = SIGN_TRANSACTION_UI_STATE_TX_VALUE;
            // Fill screen with data
            // Move to the next step, which will display the screen.
            DISPLAY_TX_STATE(ctx, ux_flow_next);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TOKEN_ID: {
            if (ctx->token_idx == 0) {
                ctx->state = SIGN_TRANSACTION_UI_STATE_TX_FEE;
            } else {
                ctx->state = SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE;
                ctx->token_idx--;
            }
            // Fill screen with data
            // Move to the next step, which will display the screen.
            DISPLAY_TX_STATE(ctx, ux_flow_next);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE: {
            ctx->state = SIGN_TRANSACTION_UI_STATE_TOKEN_ID;
            // Fill screen with data
            // Move to the next step, which will display the screen.
            DISPLAY_TX_STATE(ctx, ux_flow_next);
            break;
        }
    }
}

// Step with icon and text
UX_STEP_NOCB(ux_stx_display_tx_confirm_step, pn, {&C_icon_warning, "Confirm Transaction"});
UX_STEP_INIT(ux_stx_dynamic_upper_delimiter_step, NULL, NULL, { ui_stx_dynamic_step_left(); });
UX_STEP_INIT(ux_stx_dynamic_lower_delimiter_step, NULL, NULL, { ui_stx_dynamic_step_right(); });

// Custom dynamic step
static ux_layout_bnnn_paging_params_t G_ui_stx_dynamic_step_params[1];
const ux_flow_step_t ux_stx_dynamic_step = {ux_layout_bnnn_paging_init,
                                            G_ui_stx_dynamic_step_params,
                                            NULL,
                                            NULL};

static NOINLINE void ui_stx_operation_execute_action(bool approved) {
    G_context.is_ui_busy = false;
    sign_transaction_ui_confirm_ctx_t* ctx =
        (sign_transaction_ui_confirm_ctx_t*) CONTEXT(G_context).ui_context;
    if (approved) {
        ctx->op_response_cb(ctx->op_cb_context);
    } else {
        res_deny();
    }
    clear_context(&G_context, CMD_NONE);
    ui_menu_main();
}

bool ui_stx_add_transaction_screens(sign_transaction_ui_confirm_ctx_t* ctx,
                                    uint8_t* screen,
                                    const sign_transaction_amounts_ctx_t* amounts,
                                    uint8_t op_screen_count,
                                    ui_sign_transaction_operation_show_screen_cb screen_cb,
                                    ui_sign_transaction_operation_send_response_cb response_cb,
                                    void* cb_context) {
    if (MAX_NUMBER_OF_SCREENS - *screen < 6) return false;

    memset(ctx, 0, sizeof(sign_transaction_ui_confirm_ctx_t));

    G_ui_stx_dynamic_step_params[0].text = ctx->text;
    G_ui_stx_dynamic_step_params[0].title = ctx->title;

    G_ux_flow[(*screen)++] = &ux_stx_display_tx_confirm_step;
    G_ux_flow[(*screen)++] = &ux_stx_dynamic_upper_delimiter_step;
    G_ux_flow[(*screen)++] = &ux_stx_dynamic_step;
    G_ux_flow[(*screen)++] = &ux_stx_dynamic_lower_delimiter_step;

    const ux_flow_step_t** approve = &G_ux_flow[(*screen)++];
    const ux_flow_step_t** reject = &G_ux_flow[(*screen)++];
    ui_approve_reject_screens(ui_stx_operation_execute_action, approve, reject);

    ctx->state = SIGN_TRANSACTION_UI_STATE_NONE;
    ctx->op_screen_count = op_screen_count;
    ctx->op_screen_cb = screen_cb;
    ctx->op_response_cb = response_cb;
    ctx->op_cb_context = cb_context;
    ctx->token_idx = 0;
    ctx->amounts = amounts;

    return true;
}

bool ui_stx_display_screens(uint8_t screen_count, void* ui_context) {
    if (MAX_NUMBER_OF_SCREENS - screen_count < 2) return false;

    G_ux_flow[screen_count++] = FLOW_LOOP;
    G_ux_flow[screen_count++] = FLOW_END_STEP;

    ux_flow_init(0, G_ux_flow, NULL);

    G_context.is_ui_busy = true;
    CONTEXT(G_context).ui_context = ui_context;

    return true;
}
