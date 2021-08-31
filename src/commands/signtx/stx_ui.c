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
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_menu.h"

#define CONTEXT(gctx)            gctx.ctx.sign_tx
#define APP_ID_UI_CONTEXT(gctx)  CONTEXT(gctx).ui_app_id
#define CONFIRM_UI_CONTEXT(gctx) CONTEXT(gctx).ui_confirm

// Step with icon and text
UX_STEP_NOCB(ux_stx_display_token_confirm_step, pn, {&C_icon_processing, "Accept Tx Data"});

int ui_display_sign_tx_access_token(uint32_t app_access_token) {
    uint8_t screen = 0;
    G_ux_flow[screen++] = &ux_stx_display_token_confirm_step;

    if (app_access_token != 0) {
        G_ux_flow[screen++] =
            ui_application_id_screen(app_access_token, APP_ID_UI_CONTEXT(G_context).app_token);
    }
    APP_ID_UI_CONTEXT(G_context).app_token_value = app_access_token;

    const ux_flow_step_t** approve = &G_ux_flow[screen++];
    const ux_flow_step_t** reject = &G_ux_flow[screen++];
    ui_approve_reject_screens(ui_action_sign_tx_token, approve, reject);

    G_ux_flow[screen++] = FLOW_LOOP;
    G_ux_flow[screen++] = FLOW_END_STEP;

    ux_flow_init(0, G_ux_flow, NULL);

    G_context.is_ui_busy = true;

    return 0;
}

void ui_action_sign_tx_token(bool approved) {
    G_context.is_ui_busy = false;

    if (approved) {
        G_context.app_session_id = APP_ID_UI_CONTEXT(G_context).app_token_value;
        CONTEXT(G_context).state = SIGN_TRANSACTION_STATE_DATA_APPROVED;
        send_response_sign_transaction_session_id(CONTEXT(G_context).session);
    } else {
        res_deny();
    }

    ui_menu_main();
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

char* u64toa(uint64_t value, char string[], uint8_t size) {
    uint8_t indx = 0;
    do {
        if (indx == size - 2) return NULL;
        string[indx++] = value % 10 + '0';
    } while ((value /= 10) > 0);
    string[indx] = '\0';
    char c;
    for (uint8_t i = 0, j = indx - 1; i < j; i++, j--) {
        c = string[i];
        string[i] = string[j];
        string[j] = c;
    }
    return string + indx;
}

static NOINLINE void ui_stx_display_state() {
    char* title = CONFIRM_UI_CONTEXT(G_context).title;
    char* text = CONFIRM_UI_CONTEXT(G_context).text;
    uint8_t title_len = MEMBER_SIZE(_sign_transaction_confirm_ui_ctx_t, title);
    uint8_t text_len = MEMBER_SIZE(_sign_transaction_confirm_ui_ctx_t, text);
    memset(title, 0, title_len);
    memset(text, 0, text_len);
    switch (CONFIRM_UI_CONTEXT(G_context).state) {
        case SIGN_TRANSACTION_UI_STATE_TX_VALUE: {
            strncpy(title, "Transaction Amount", title_len);
            uint64_t value = CONTEXT(G_context).amounts.inputs;
            if (!checked_sub_u64(value, CONTEXT(G_context).amounts.fee, &value) ||
                !checked_sub_u64(value, CONTEXT(G_context).amounts.change, &value)) {
                strncpy(text, "Bad TX. Outputs is bigger than inputs", text_len);
            } else {
                u64toa(value, text, text_len);
            }
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TX_FEE:
            strncpy(title, "Transaction Fee", title_len);
            u64toa(CONTEXT(G_context).amounts.fee, text, text_len);
            break;
        case SIGN_TRANSACTION_UI_STATE_TOKEN_ID: {
            uint8_t token_idx = CONFIRM_UI_CONTEXT(G_context).token_idx;
            snprintf(title, title_len, "Token %u", (unsigned int) token_idx + 1);
            base58_encode(CONTEXT(G_context).tokens_table.tokens[token_idx],
                          ERGO_ID_LEN,
                          text,
                          text_len);
            break;
        }
        case SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE: {
            uint8_t token_idx = CONFIRM_UI_CONTEXT(G_context).token_idx;
            snprintf(title, title_len, "Token %u", (unsigned int) token_idx + 1);
            _sign_transaction_token_amount_t* amount =
                &CONTEXT(G_context).amounts.tokens[token_idx];
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
                    strncpy(text, "Minting: ", text_len);
                    text_len -= sizeof("Minting: ") - 1;
                    text += sizeof("Minting: ");
                } else {
                    strncpy(text, "Burning: ", text_len);
                    text_len -= sizeof("Burning: ") - 1;
                    text += sizeof("Burning: ");
                }
                char* u64 = u64toa(value, text, text_len);
                text_len -= (u64 - text) - 1;
                strncpy(u64, ";\n", text_len);
                text = u64 + 2;
                text_len -= 2;
            }
            strncpy(text, "Sending: ", text_len);
            text_len -= sizeof("Sending: ") - 1;
            text += sizeof("Sending: ");
            u64toa(amount->output, text, text_len);
            break;
        }
        default:
            break;
    }
}

static inline void ui_stx_dynamic_step_right() {
    switch (CONFIRM_UI_CONTEXT(G_context).state) {
        case SIGN_TRANSACTION_UI_STATE_NONE:
            if (CONTEXT(G_context).tokens_table.count > 0) {
                // TX has tokens. Show last token value
                CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE;
                CONFIRM_UI_CONTEXT(G_context).token_idx = CONTEXT(G_context).tokens_table.count - 1;
            } else {
                // TX doesn't have tokens. Show TX fee value
                CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TX_FEE;
                CONFIRM_UI_CONTEXT(G_context).token_idx = 0;
            }
            // Fill screen with data
            ui_stx_display_state();
            // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s weird
            // behaviour.
            bnnn_paging_edgecase();
            break;
        case SIGN_TRANSACTION_UI_STATE_TX_VALUE:
            CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TX_FEE;
            // Fill screen with data
            ui_stx_display_state();
            // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s weird
            // behaviour.
            bnnn_paging_edgecase();
            break;
        case SIGN_TRANSACTION_UI_STATE_TX_FEE:
            if (CONTEXT(G_context).tokens_table.count > 0) {
                CONFIRM_UI_CONTEXT(G_context).token_idx = 0;
                CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TOKEN_ID;
                // Fill screen with data
                ui_stx_display_state();
                // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s
                // weird behaviour.
                bnnn_paging_edgecase();
            } else {
                CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_NONE;
                // go to the next static screen
                ux_flow_next();
            }
            break;
        case SIGN_TRANSACTION_UI_STATE_TOKEN_ID:
            CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE;
            // Fill screen with data
            ui_stx_display_state();
            // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s weird
            // behaviour.
            bnnn_paging_edgecase();
        case SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE:
            if (CONFIRM_UI_CONTEXT(G_context).token_idx <
                CONTEXT(G_context).tokens_table.count - 1) {
                CONFIRM_UI_CONTEXT(G_context).token_idx++;
                CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TOKEN_ID;
                // Fill screen with data
                ui_stx_display_state();
                // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s
                // weird behaviour.
                bnnn_paging_edgecase();
            } else {
                CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_NONE;
                // go to the next static screen
                ux_flow_next();
            }
    }
}

static inline void ui_stx_dynamic_step_left() {
    switch (CONFIRM_UI_CONTEXT(G_context).state) {
        case SIGN_TRANSACTION_UI_STATE_NONE:
            // Show TX ERG value
            CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TX_VALUE;
            // Fill screen with data
            ui_stx_display_state();
            // Move to the next step, which will display the screen.
            ux_flow_next();
            break;
        case SIGN_TRANSACTION_UI_STATE_TX_VALUE:
            CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_NONE;
            // Similar to `ux_flow_prev()` but updates layout to account for `bnnn_paging`'s weird
            // behaviour.
            bnnn_paging_edgecase();
            break;
        case SIGN_TRANSACTION_UI_STATE_TX_FEE:
            CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TX_VALUE;
            // Fill screen with data
            ui_stx_display_state();
            // Move to the next step, which will display the screen.
            ux_flow_next();
            break;
        case SIGN_TRANSACTION_UI_STATE_TOKEN_ID:
            if (CONFIRM_UI_CONTEXT(G_context).token_idx == 0) {
                CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TX_FEE;
            } else {
                CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE;
                CONFIRM_UI_CONTEXT(G_context).token_idx--;
            }
            // Fill screen with data
            ui_stx_display_state();
            // Move to the next step, which will display the screen.
            ux_flow_next();
            break;
        case SIGN_TRANSACTION_UI_STATE_TOKEN_VALUE:
            CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_TOKEN_ID;
            // Fill screen with data
            ui_stx_display_state();
            // Move to the next step, which will display the screen.
            ux_flow_next();
            break;
    }
}

// Step with icon and text
UX_STEP_NOCB(ux_stx_display_tx_confirm_step, pn, {&C_icon_warning, "Confirm Transaction"});
UX_STEP_INIT(ux_stx_dynamic_upper_delimiter_step, NULL, NULL, { ui_stx_dynamic_step_left(); });
UX_STEP_INIT(ux_stx_dynamic_lower_delimiter_step, NULL, NULL, { ui_stx_dynamic_step_right(); });
UX_STEP_NOCB(ux_stx_dynamic_step,
             bnnn_paging,
             {.title = CONFIRM_UI_CONTEXT(G_context).title,
              .text = CONFIRM_UI_CONTEXT(G_context).text});

int ui_display_sign_tx_transaction(void) {
    memset(&CONFIRM_UI_CONTEXT(G_context), 0, sizeof(_sign_transaction_confirm_ui_ctx_t));
    CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_NONE;

    uint8_t screen = 0;
    G_ux_flow[screen++] = &ux_stx_display_tx_confirm_step;
    G_ux_flow[screen++] = &ux_stx_dynamic_upper_delimiter_step;
    G_ux_flow[screen++] = &ux_stx_dynamic_step;
    G_ux_flow[screen++] = &ux_stx_dynamic_lower_delimiter_step;

    const ux_flow_step_t** approve = &G_ux_flow[screen++];
    const ux_flow_step_t** reject = &G_ux_flow[screen++];
    ui_approve_reject_screens(ui_action_sign_tx_transaction, approve, reject);

    G_ux_flow[screen++] = FLOW_LOOP;
    G_ux_flow[screen++] = FLOW_END_STEP;

    CONFIRM_UI_CONTEXT(G_context).state = SIGN_TRANSACTION_UI_STATE_NONE;
    CONFIRM_UI_CONTEXT(G_context).token_idx = 0;

    ux_flow_init(0, G_ux_flow, NULL);

    G_context.is_ui_busy = true;

    return 0;
}

void ui_action_sign_tx_transaction(bool approved) {
    G_context.is_ui_busy = false;

    if (approved) {
        CONTEXT(G_context).state = SIGN_TRANSACTION_STATE_CONFIRMED;
        res_ok();
    } else {
        res_deny();
    }

    ui_menu_main();
}
