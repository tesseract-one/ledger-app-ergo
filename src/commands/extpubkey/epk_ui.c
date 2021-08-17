#include <os.h>
#include <ux.h>

#include "epk_ui.h"
#include "epk_response.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../context.h"
#include "../../io.h"
#include "../../sw.h"
#include "../../menu.h"
#include "../../common/bip32.h"
#include "../../common/macros.h"

// Step with icon and text
UX_STEP_NOCB(ux_epk_display_confirm_ext_pubkey_step, pn, {&C_icon_eye, "Confirm Ext PubKey"});
// Step with title/text for account number
UX_STEP_NOCB(ux_epk_display_account_step,
             bnnn_paging,
             {
                 .title = "Account",
                 .text = G_ui_ctx.ext_pub_key.account,
             });
// Step with title/text for application token
UX_STEP_NOCB(ux_epk_display_app_token_step,
             bnnn_paging,
             {
                 .title = "Application",
                 .text = G_ui_ctx.ext_pub_key.app_token,
             });
// Step with approve button
UX_STEP_CB(ux_epk_display_approve_step,
           pb,
           ui_action_get_extended_pubkey(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_epk_display_reject_step,
           pb,
           ui_action_get_extended_pubkey(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display address and BIP32 path:
// #1 screen: eye icon + "Confirm Address"
// #2 screen: display account number
// #3 screen: display application token
// #4 screen: approve button
// #5 screen: reject button
UX_FLOW(ux_epk_display_confirm_ext_pubkey_flow,
        &ux_epk_display_confirm_ext_pubkey_step,
        &ux_epk_display_account_step,
        &ux_epk_display_app_token_step,
        &ux_epk_display_approve_step,
        &ux_epk_display_reject_step,
        FLOW_LOOP);

int ui_display_account(uint32_t app_access_token) {
    if (G_context.current_command != CMD_GET_EXTENDED_PUBLIC_KEY) {
        return io_send_sw(SW_BAD_STATE);
    }

    if (!bip32_path_validate(G_context.ext_pub_ctx.bip32_path,
                             G_context.ext_pub_ctx.bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ACCOUNT_GE3)) {
        return io_send_sw(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    G_ui_ctx.ext_pub_key.app_token_value = app_access_token;
    explicit_bzero(G_ui_ctx.ext_pub_key.account, MEMBER_SIZE(extended_public_key_ui_ctx_t, account));
    snprintf(
        G_ui_ctx.ext_pub_key.account,
        MEMBER_SIZE(extended_public_key_ui_ctx_t, account),
        "%d",
        G_context.ext_pub_ctx.bip32_path[2] - BIP32_HARDENED_CONSTANT);

    explicit_bzero(G_ui_ctx.ext_pub_key.app_token, MEMBER_SIZE(extended_public_key_ui_ctx_t, app_token));
    snprintf(G_ui_ctx.ext_pub_key.app_token,
             MEMBER_SIZE(extended_public_key_ui_ctx_t, app_token),
             "0x%x",
             app_access_token);

    ux_flow_init(0, ux_epk_display_confirm_ext_pubkey_flow, NULL);

    G_context.is_ui_busy = true;

    return 0;
}

void ui_action_get_extended_pubkey(bool choice) {
    G_context.is_ui_busy = false;

    if (choice) {
        G_context.app_session_id = G_ui_ctx.ext_pub_key.app_token_value;
        send_response_extended_pubkey();
    } else {
        clear_context(&G_context, CMD_NONE);
        io_send_sw(SW_DENY);
    }

    ui_menu_main();
}
