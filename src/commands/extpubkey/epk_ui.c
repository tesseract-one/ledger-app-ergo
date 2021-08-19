#include <os.h>
#include <ux.h>
#include <string.h>

#include "epk_ui.h"
#include "epk_response.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../context.h"
#include "../../sw.h"
#include "../../menu.h"
#include "../../common/bip32.h"
#include "../../common/macros.h"
#include "../../helpers/response.h"

#define UI_CONTEXT(gctx) gctx.ui.ext_pub_key

// Step with icon and text
UX_STEP_NOCB(ux_epk_display_confirm_ext_pubkey_step, pn, {&C_icon_eye, "Confirm Ext PubKey"});
// Step with title/text for account number
UX_STEP_NOCB(ux_epk_display_account_step,
             bn,
             {
                 .line1 = "Account",
                 .line2 = UI_CONTEXT(G_context).account,
             });
// Step with title/text for application token
UX_STEP_NOCB(ux_epk_display_app_token_step,
             bn,
             {
                 .line1 = "Application",
                 .line2 = UI_CONTEXT(G_context).app_token,
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

int ui_display_account(uint32_t app_access_token,
                       uint32_t *bip32_path,
                       uint8_t bip32_path_len,
                       uint8_t raw_pub_key[static PUBLIC_KEY_LEN],
                       uint8_t chain_code[static CHAIN_CODE_LEN]) {
    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ACCOUNT_GE3)) {
        return res_error(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    UI_CONTEXT(G_context).app_token_value = app_access_token;
    memset(UI_CONTEXT(G_context).account, 0, MEMBER_SIZE(extended_public_key_ui_ctx_t, account));
    snprintf(UI_CONTEXT(G_context).account,
             MEMBER_SIZE(extended_public_key_ui_ctx_t, account),
             "%d",
             bip32_path[2] & (BIP32_HARDENED_CONSTANT - 1));

    memset(UI_CONTEXT(G_context).app_token,
           0,
           MEMBER_SIZE(extended_public_key_ui_ctx_t, app_token));
    snprintf(UI_CONTEXT(G_context).app_token,
             MEMBER_SIZE(extended_public_key_ui_ctx_t, app_token),
             "0x%08x",
             app_access_token);

    memmove(UI_CONTEXT(G_context).raw_public_key, raw_pub_key, PUBLIC_KEY_LEN);
    memmove(UI_CONTEXT(G_context).chain_code, chain_code, CHAIN_CODE_LEN);

    ux_flow_init(0, ux_epk_display_confirm_ext_pubkey_flow, NULL);

    G_context.ui.is_busy = true;

    return 0;
}

void ui_action_get_extended_pubkey(bool choice) {
    G_context.ui.is_busy = false;

    if (choice) {
        G_context.app_session_id = UI_CONTEXT(G_context).app_token_value;
        send_response_extended_pubkey(UI_CONTEXT(G_context).raw_public_key,
                                      UI_CONTEXT(G_context).chain_code);
        explicit_bzero(&UI_CONTEXT(G_context), sizeof(extended_public_key_ui_ctx_t));
    } else {
        explicit_bzero(&UI_CONTEXT(G_context), sizeof(extended_public_key_ui_ctx_t));
        res_deny();
    }

    ui_menu_main();
}
