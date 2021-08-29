#include <os.h>
#include <ux.h>
#include <string.h>

#include "epk_ui.h"
#include "epk_response.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../context.h"
#include "../../sw.h"
#include "../../common/bip32.h"
#include "../../common/macros.h"
#include "../../helpers/response.h"
#include "../../ui/ui_bip32_path.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_menu.h"

#define CONTEXT(gctx) gctx.ctx.ext_pub_key

// Step with icon and text
UX_STEP_NOCB(ux_epk_display_confirm_ext_pubkey_step, pn, {&C_icon_warning, "Ext PubKey Export"});

int ui_display_account(uint32_t app_access_token,
                       uint32_t* bip32_path,
                       uint8_t bip32_path_len,
                       uint8_t raw_pub_key[static PUBLIC_KEY_LEN],
                       uint8_t chain_code[static CHAIN_CODE_LEN]) {
    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ACCOUNT_GE3)) {
        return res_error(SW_BIP32_BAD_PATH);
    }

    uint8_t screen = 0;
    G_ux_flow[screen++] = &ux_epk_display_confirm_ext_pubkey_step;

    const ux_flow_step_t* b32_step =
        ui_bip32_path_screen(bip32_path,
                             bip32_path_len,
                             CONTEXT(G_context).bip32_path,
                             MEMBER_SIZE(extended_public_key_ctx_t, bip32_path));
    if (b32_step == NULL) {
        return res_error(SW_BIP32_FORMATTING_FAILED);
    }
    G_ux_flow[screen++] = b32_step;

    if (app_access_token != 0) {
        G_ux_flow[screen++] =
            ui_application_id_screen(app_access_token, CONTEXT(G_context).app_token);
    }

    const ux_flow_step_t** approve = &G_ux_flow[screen++];
    const ux_flow_step_t** reject = &G_ux_flow[screen++];
    ui_approve_reject_screens(&ui_action_get_extended_pubkey, approve, reject);

    G_ux_flow[screen++] = FLOW_LOOP;
    G_ux_flow[screen++] = FLOW_END_STEP;

    CONTEXT(G_context).app_token_value = app_access_token;
    memmove(CONTEXT(G_context).raw_public_key, raw_pub_key, PUBLIC_KEY_LEN);
    memmove(CONTEXT(G_context).chain_code, chain_code, CHAIN_CODE_LEN);

    ux_flow_init(0, G_ux_flow, NULL);

    G_context.is_ui_busy = true;

    return 0;
}

void ui_action_get_extended_pubkey(bool choice) {
    G_context.is_ui_busy = false;

    if (choice) {
        G_context.app_session_id = CONTEXT(G_context).app_token_value;
        send_response_extended_pubkey(CONTEXT(G_context).raw_public_key,
                                      CONTEXT(G_context).chain_code);
        explicit_bzero(&CONTEXT(G_context), sizeof(extended_public_key_ctx_t));
    } else {
        explicit_bzero(&CONTEXT(G_context), sizeof(extended_public_key_ctx_t));
        res_deny();
    }

    ui_menu_main();
}
