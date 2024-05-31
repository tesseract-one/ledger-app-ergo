#include <os.h>
#include <ux.h>
#include <string.h>
#include <glyphs.h>

#include "epk_ui.h"
#include "epk_response.h"
#include "../../context.h"
#include "../../sw.h"
#include "../../common/bip32_ext.h"
#include "../../common/macros_ext.h"
#include "../../helpers/response.h"
#include "../../ui/ui_bip32_path.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_menu.h"
#include "../../ui/ui_main.h"

// Step with icon and text
UX_STEP_NOCB(ux_epk_display_confirm_ext_pubkey_step, pn, {&C_icon_warning, "Ext PubKey Export"});

static NOINLINE void ui_action_get_extended_pubkey(bool approved, void* context) {
    extended_public_key_ctx_t* ctx = (extended_public_key_ctx_t*) context;
    app_set_ui_busy(false);

    if (approved) {
        app_set_connected_app_id(ctx->app_token_value);
        send_response_extended_pubkey(ctx->raw_public_key, ctx->chain_code);
        explicit_bzero(ctx, sizeof(extended_public_key_ctx_t));
    } else {
        explicit_bzero(ctx, sizeof(extended_public_key_ctx_t));
        res_deny();
    }

    app_set_current_command(CMD_NONE);

    ui_menu_main();
}

int ui_display_account(extended_public_key_ctx_t* ctx,
                       uint32_t app_access_token,
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
    ui_add_screen(&ux_epk_display_confirm_ext_pubkey_step, &screen);

    const ux_flow_step_t* b32_step =
        ui_bip32_path_screen(bip32_path,
                             bip32_path_len,
                             "Path",
                             ctx->bip32_path,
                             MEMBER_SIZE(extended_public_key_ctx_t, bip32_path),
                             NULL,
                             NULL);
    if (b32_step == NULL) {
        app_set_current_command(CMD_NONE);
        return res_error(SW_BIP32_FORMATTING_FAILED);
    }
    ui_add_screen(b32_step, &screen);

    if (app_access_token != 0) {
        ui_add_screen(ui_application_id_screen(app_access_token, ctx->app_token), &screen);
    }

    ui_approve_reject_screens(ui_action_get_extended_pubkey,
                              ctx,
                              ui_next_sreen_ptr(&screen),
                              ui_next_sreen_ptr(&screen));

    ctx->app_token_value = app_access_token;
    memmove(ctx->raw_public_key, raw_pub_key, PUBLIC_KEY_LEN);
    memmove(ctx->chain_code, chain_code, CHAIN_CODE_LEN);

    ui_display_screens(&screen);

    return 0;
}
