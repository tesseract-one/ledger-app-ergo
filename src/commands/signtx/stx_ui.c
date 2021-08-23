#include <os.h>
#include <ux.h>

#include "stx_ui.h"
#include "stx_response.h"
#include "stx_sw.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../menu.h"
#include "../../common/macros.h"
#include "../../helpers/response.h"
#include "../../common/int_ops.h"

#define UI_CONTEXT(gctx) gctx.ui.sign_tx

// Step with icon and text
UX_STEP_NOCB(ux_stx_display_token_confirm_step, pn, {&C_icon_processing, "Accept Tx Data"});
// Step with title/text for application token
UX_STEP_NOCB(ux_stx_display_app_token_step,
             bn,
             {
                 .line1 = "Application",
                 .line2 = UI_CONTEXT(G_context).value,
             });
// Step with approve button
UX_STEP_CB(ux_stx_display_token_approve_step,
           pb,
           ui_action_sign_tx_token(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_stx_display_token_reject_step,
           pb,
           ui_action_sign_tx_token(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display application token:
// #1 screen: eye icon + "Accept Tx Data"
// #2 screen: display application token
// #3 screen: approve button
// #4 screen: reject button
UX_FLOW(ux_stx_display_app_token_flow,
        &ux_stx_display_token_confirm_step,
        &ux_stx_display_app_token_step,
        &ux_stx_display_token_approve_step,
        &ux_stx_display_token_reject_step,
        FLOW_LOOP);

int ui_display_sign_tx_access_token(uint32_t app_access_token) {
    UI_CONTEXT(G_context).app_token_value = app_access_token;

    memset(UI_CONTEXT(G_context).value, 0, MEMBER_SIZE(sign_transaction_ui_ctx_t, value));
    snprintf(UI_CONTEXT(G_context).value,
             MEMBER_SIZE(sign_transaction_ui_ctx_t, value),
             "0x%08x",
             app_access_token);

    ux_flow_init(0, ux_stx_display_app_token_flow, NULL);

    G_context.ui.is_busy = true;

    return 0;
}

// Step with icon and text
UX_STEP_NOCB(ux_stx_display_tx_confirm_step, pn, {&C_icon_warning, "Confirm Transaction"});
// Step with title/text for tx value
UX_STEP_NOCB(ux_stx_display_tx_value_step,
             bnnn_paging,
             {
                 .title = "Ergo Value",
                 .text = UI_CONTEXT(G_context).value,
             });
UX_STEP_NOCB(ux_stx_display_tx_fee_step,
             bnnn_paging,
             {
                 .title = "Fee",
                 .text = UI_CONTEXT(G_context).fee,
             });
// Step with approve button
UX_STEP_CB(ux_stx_display_tx_approve_step,
           pb,
           ui_action_sign_tx_transaction(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_stx_display_tx_reject_step,
           pb,
           ui_action_sign_tx_transaction(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display application token:
// #1 screen: eye icon + "Accept Tx Data"
// #2 screen: display application token
// #3 screen: approve button
// #4 screen: reject button
UX_FLOW(ux_stx_display_transaction_flow,
        &ux_stx_display_tx_confirm_step,
        &ux_stx_display_tx_value_step,
        &ux_stx_display_tx_fee_step,
        &ux_stx_display_tx_approve_step,
        &ux_stx_display_tx_reject_step,
        FLOW_LOOP);

bool u64toa(uint64_t value, char string[], uint8_t size) {
    uint8_t indx = 0;
    do {
        if (indx == size - 2) return false;
        string[indx++] = value % 10 + '0';
    } while ((value /= 10) > 0);
    string[indx] = '\0';
    char c;
    for (uint8_t i = 0, j = indx - 1; i < j; i++, j--) {
        c = string[i];
        string[i] = string[j];
        string[j] = c;
    }
    return true;
}

int ui_display_sign_tx_transaction(void) {
    uint64_t value = UI_CONTEXT(G_context).inputs_value;

    checked_sub_u64(value, UI_CONTEXT(G_context).fee_value, &value);
    checked_sub_u64(value, UI_CONTEXT(G_context).change_value, &value);

    memset(UI_CONTEXT(G_context).value, 0, MEMBER_SIZE(sign_transaction_ui_ctx_t, value));
    u64toa(value, UI_CONTEXT(G_context).value, MEMBER_SIZE(sign_transaction_ui_ctx_t, value));

    memset(UI_CONTEXT(G_context).fee, 0, MEMBER_SIZE(sign_transaction_ui_ctx_t, fee));
    u64toa(UI_CONTEXT(G_context).fee_value,
           UI_CONTEXT(G_context).fee,
           MEMBER_SIZE(sign_transaction_ui_ctx_t, fee));

    ux_flow_init(0, ux_stx_display_transaction_flow, NULL);

    G_context.ui.is_busy = true;

    return 0;
}

void ui_action_sign_tx_token(bool choice) {
    G_context.ui.is_busy = false;

    if (choice) {
        G_context.app_session_id = UI_CONTEXT(G_context).app_token_value;
        G_context.sign_tx_ctx.state = SIGN_TRANSACTION_STATE_DATA_APPROVED;
        send_response_sign_transaction_session_id(G_context.sign_tx_ctx.session);
    } else {
        res_deny();
    }

    ui_menu_main();
}

void ui_action_sign_tx_transaction(bool approved) {
    G_context.ui.is_busy = false;

    if (approved) {
        G_context.sign_tx_ctx.state = SIGN_TRANSACTION_STATE_CONFIRMED;
        res_ok();
    } else {
        res_deny();
    }

    ui_menu_main();
}
