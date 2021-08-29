#include <os.h>
#include <ux.h>

#include "da_ui.h"
#include "da_response.h"
#include "../../sw.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../context.h"
#include "../../common/base58.h"
#include "../../common/macros.h"
#include "../../ergo/address.h"
#include "../../helpers/response.h"
#include "../../ui/ui_bip32_path.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_menu.h"

#define CONTEXT(gctx) gctx.ctx.derive_address

// Step with icon and text
UX_STEP_NOCB(ux_da_display_confirm_addr_step, pn, {&C_icon_eye, "Confirm Address"});
UX_STEP_NOCB(ux_da_display_confirm_send_step, pn, {&C_icon_processing, "Confirm Send Address"});
// Step with title/text for address
UX_STEP_NOCB(ux_da_display_address_step,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = CONTEXT(G_context).address,
             });

// Display
int ui_display_address(bool send,
                       uint8_t network_id,
                       uint32_t app_access_token,
                       uint32_t* bip32_path,
                       uint8_t bip32_path_len,
                       uint8_t raw_pub_key[static PUBLIC_KEY_LEN]) {
    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return res_error(SW_BIP32_BAD_PATH);
    }

    CONTEXT(G_context).app_token_value = app_access_token;
    CONTEXT(G_context).send = send;

    uint8_t screen = 0;
    G_ux_flow[screen++] =
        send ? &ux_da_display_confirm_send_step : &ux_da_display_confirm_addr_step;

    const ux_flow_step_t* b32_screen =
        ui_bip32_path_screen(bip32_path,
                             bip32_path_len,
                             CONTEXT(G_context).bip32_path,
                             MEMBER_SIZE(derive_address_ctx_t, bip32_path));
    if (b32_screen == NULL) {
        return res_error(SW_BIP32_FORMATTING_FAILED);
    }

    G_ux_flow[screen++] = b32_screen;

    memset(CONTEXT(G_context).address, 0, MEMBER_SIZE(derive_address_ctx_t, address));
    if (!send) {
        uint8_t address[ADDRESS_LEN] = {0};
        if (!address_from_pubkey(network_id, raw_pub_key, address)) {
            return res_error(SW_ADDRESS_GENERATION_FAILED);
        }

        int result = base58_encode(address,
                                   sizeof(address),
                                   CONTEXT(G_context).address,
                                   MEMBER_SIZE(derive_address_ctx_t, address));

        if (result == -1 || result >= ADDRESS_STRING_MAX_LEN) {
            return res_error(SW_ADDRESS_FORMATTING_FAILED);
        }
        G_ux_flow[screen++] = &ux_da_display_address_step;
    }

    if (app_access_token != 0) {
        G_ux_flow[screen++] = ui_application_id_screen(app_access_token, CONTEXT(G_context).app_id);
    }

    const ux_flow_step_t** approve = &G_ux_flow[screen++];
    const ux_flow_step_t** reject = &G_ux_flow[screen++];
    ui_approve_reject_screens(&ui_action_derive_address, approve, reject);

    G_ux_flow[screen++] = FLOW_LOOP;
    G_ux_flow[screen++] = FLOW_END_STEP;

    memmove(CONTEXT(G_context).raw_public_key, raw_pub_key, PUBLIC_KEY_LEN);

    ux_flow_init(0, G_ux_flow, NULL);

    G_context.is_ui_busy = true;

    return 0;
}

// Action
void ui_action_derive_address(bool choice) {
    G_context.is_ui_busy = true;

    if (choice) {
        G_context.app_session_id = CONTEXT(G_context).app_token_value;
        if (CONTEXT(G_context).send) {
            send_response_address(CONTEXT(G_context).raw_public_key);
        } else {
            clear_context(&G_context, CMD_NONE);
            res_ok();
        }
    } else {
        res_deny();
    }

    ui_menu_main();
}