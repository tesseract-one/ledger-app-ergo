#include <os.h>
#include <ux.h>

#include "da_ui.h"
#include "da_response.h"
#include "../../sw.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../context.h"
#include "../../menu.h"
#include "../../common/base58.h"
#include "../../common/macros.h"
#include "../../ergo/address.h"
#include "../../helpers/response.h"

#define UI_CONTEXT(gcxt) G_context.ui.derive_address

// Step with icon and text
UX_STEP_NOCB(ux_da_display_confirm_addr_step, pn, {&C_icon_eye, "Confirm Address"});
UX_STEP_NOCB(ux_da_display_confirm_send_step, pn, {&C_icon_processing, "Confirm Send Address"});
// Step with title/text for account number
UX_STEP_NOCB(ux_da_display_path_step,
             bnnn_paging,
             {
                 .title = "Path",
                 .text = UI_CONTEXT(G_context).bip32_path,
             });
// Step with title/text for address
UX_STEP_NOCB(ux_da_display_address_step,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = UI_CONTEXT(G_context).address,
             });
// Step with title/text for application token
UX_STEP_NOCB(ux_da_display_app_token_step,
             bn,
             {
                 .line1 = "Application",
                 .line2 = UI_CONTEXT(G_context).app_token,
             });
// Step with approve button
UX_STEP_CB(ux_da_display_approve_step,
           pb,
           ui_action_derive_address(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_da_display_reject_step,
           pb,
           ui_action_derive_address(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display address and BIP32 path:
// #1 screen: eye icon + "Confirm Address"
// #2 screen: display Bip32 path
// #3 screen: display address
// #4 screen: display application token
// #5 screen: approve button
// #6 screen: reject button
UX_FLOW(ux_da_display_address_flow,
        &ux_da_display_confirm_addr_step,
        &ux_da_display_path_step,
        &ux_da_display_address_step,
        &ux_da_display_app_token_step,
        &ux_da_display_approve_step,
        &ux_da_display_reject_step,
        FLOW_LOOP);

// FLOW to display BIP32 path and send:
// #1 screen: eye icon + "Confirm Send Address"
// #2 screen: display Bip32 path
// #3 screen: display application token
// #4 screen: approve button
// #5 screen: reject button
UX_FLOW(ux_da_send_address_flow,
        &ux_da_display_confirm_send_step,
        &ux_da_display_path_step,
        &ux_da_display_app_token_step,
        &ux_da_display_approve_step,
        &ux_da_display_reject_step,
        FLOW_LOOP);

// Display
int ui_display_address(bool send,
                       uint8_t network_id,
                       uint32_t app_access_token,
                       uint32_t *bip32_path,
                       uint8_t bip32_path_len,
                       uint8_t raw_pub_key[static PUBLIC_KEY_LEN]) {
    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return res_error(SW_BIP32_BAD_PATH);
    }

    UI_CONTEXT(G_context).app_token_value = app_access_token;
    UI_CONTEXT(G_context).send = send;

    memset(UI_CONTEXT(G_context).bip32_path, 0, MEMBER_SIZE(derive_address_ui_ctx_t, bip32_path));
    if (!bip32_path_format(bip32_path,
                           bip32_path_len,
                           UI_CONTEXT(G_context).bip32_path,
                           MEMBER_SIZE(derive_address_ui_ctx_t, bip32_path))) {
        return res_error(SW_BIP32_FORMATTING_FAILED);
    }

    memset(UI_CONTEXT(G_context).address, 0, MEMBER_SIZE(derive_address_ui_ctx_t, address));
    if (!send) {
        uint8_t address[ADDRESS_LEN] = {0};
        if (!address_from_pubkey(network_id, raw_pub_key, address)) {
            return res_error(SW_ADDRESS_GENERATION_FAILED);
        }

        int result = base58_encode(address,
                                   sizeof(address),
                                   UI_CONTEXT(G_context).address,
                                   MEMBER_SIZE(derive_address_ui_ctx_t, address));

        if (result == -1 || result >= ADDRESS_STRING_MAX_LEN) {
            return res_error(SW_ADDRESS_FORMATTING_FAILED);
        }
    }

    memset(UI_CONTEXT(G_context).app_token, 0, MEMBER_SIZE(derive_address_ui_ctx_t, app_token));
    snprintf(UI_CONTEXT(G_context).app_token,
             MEMBER_SIZE(derive_address_ui_ctx_t, app_token),
             "0x%08x",
             app_access_token);

    memmove(UI_CONTEXT(G_context).raw_public_key, raw_pub_key, PUBLIC_KEY_LEN);

    if (send) {
        ux_flow_init(0, ux_da_send_address_flow, NULL);
    } else {
        ux_flow_init(0, ux_da_display_address_flow, NULL);
    }

    G_context.ui.is_busy = true;

    return 0;
}

// Action
void ui_action_derive_address(bool choice) {
    G_context.ui.is_busy = false;

    if (choice) {
        G_context.app_session_id = UI_CONTEXT(G_context).app_token_value;
        if (UI_CONTEXT(G_context).send) {
            send_response_address(UI_CONTEXT(G_context).raw_public_key);
        } else {
            clear_context(&G_context, CMD_NONE);
            res_ok();
        }
    } else {
        res_deny();
    }

    ui_menu_main();
}