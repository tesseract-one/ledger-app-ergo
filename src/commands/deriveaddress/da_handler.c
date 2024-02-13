#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memset, explicit_bzero

#include <os.h>
#include <cx.h>

#include "da_handler.h"
#include "da_ui.h"
#include "da_response.h"
#include "../../sw.h"
#include "../../context.h"
#include "../../helpers/crypto.h"
#include "../../common/rwbuffer.h"
#include "../../common/macros_ext.h"
#include "../../helpers/response.h"
#include "../../helpers/session_id.h"
#include "../../ergo/address.h"

#define COMMAND_ERROR_HANDLER handler_err
#include "../../helpers/cmd_macros.h"

static inline int handler_err(derive_address_ctx_t *_ctx, uint16_t err) {
    UNUSED(_ctx);
    app_set_current_command(CMD_NONE);
    return res_error(err);
}

int handler_derive_address(buffer_t *cdata, bool display, bool has_access_token) {
    if (app_is_ui_busy()) {
        return res_ui_busy();
    }
    app_set_current_command(CMD_DERIVE_ADDRESS);

    derive_address_ctx_t *ctx = app_derive_address_context();

    uint8_t bip32_path_len;
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint8_t public_key[PUBLIC_KEY_LEN];

    uint32_t access_token = 0;
    uint8_t network_type = 0;

    CHECK_READ_PARAM(ctx, buffer_read_u8(cdata, &network_type));
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
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return handler_err(ctx, SW_BIP32_BAD_PATH);
    }

    if (crypto_generate_public_key(bip32_path, bip32_path_len, public_key, NULL) != 0) {
        return handler_err(ctx, SW_INTERNAL_CRYPTO_ERROR);
    }

    if (!ergo_address_from_pubkey(network_type, public_key, ctx->raw_address)) {
        return res_error(SW_ADDRESS_GENERATION_FAILED);
    }

    if (!display && is_known_application(access_token, app_connected_app_id())) {
        return send_response_address(ctx->raw_address);
    }

    return ui_display_address(ctx,
                              !display,
                              access_token,
                              bip32_path,
                              bip32_path_len,
                              ctx->raw_address);
}