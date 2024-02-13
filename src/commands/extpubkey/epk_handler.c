#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memset, explicit_bzero

#include <os.h>
#include <cx.h>
#include <buffer.h>

#include "epk_handler.h"
#include "epk_ui.h"
#include "epk_response.h"
#include "../../sw.h"
#include "../../context.h"
#include "../../helpers/crypto.h"
#include "../../helpers/response.h"
#include "../../common/macros_ext.h"
#include "../../helpers/session_id.h"

#define COMMAND_ERROR_HANDLER handler_err
#include "../../helpers/cmd_macros.h"

static inline int handler_err(extended_public_key_ctx_t *_ctx, uint16_t err) {
    UNUSED(_ctx);
    app_set_current_command(CMD_NONE);
    return res_error(err);
}

int handler_get_extended_public_key(buffer_t *cdata, bool has_access_token) {
    if (app_is_ui_busy()) {
        return res_ui_busy();
    }
    app_set_current_command(CMD_GET_EXTENDED_PUBLIC_KEY);

    extended_public_key_ctx_t *ctx = app_extended_public_key_context();

    uint8_t bip32_path_len;
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint32_t access_token = 0;

    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &bip32_path_len));
    CHECK_READ_PARAM(ctx, buffer_read_bip32_path(cdata, bip32_path, (size_t) bip32_path_len));
    if (has_access_token) {
        CHECK_READ_PARAM(ctx, buffer_read_u32(cdata, &access_token, BE));
    }
    CHECK_PARAMS_FINISHED(ctx, cdata);

    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ACCOUNT_GE3)) {
        return handler_err(ctx, SW_BIP32_BAD_PATH);
    }

    if (crypto_generate_public_key(bip32_path,
                                   bip32_path_len,
                                   ctx->raw_public_key,
                                   ctx->chain_code) != 0) {
        return handler_err(ctx, SW_INTERNAL_CRYPTO_ERROR);
    }

    if (is_known_application(access_token, app_connected_app_id())) {
        return send_response_extended_pubkey(ctx->raw_public_key, ctx->chain_code);
    }

    return ui_display_account(ctx,
                              access_token,
                              bip32_path,
                              bip32_path_len,
                              ctx->raw_public_key,
                              ctx->chain_code);
}