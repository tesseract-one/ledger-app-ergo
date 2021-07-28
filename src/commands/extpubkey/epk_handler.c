#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memset, explicit_bzero

#include <os.h>
#include <cx.h>

#include "epk_handler.h"
#include "epk_ui.h"
#include "epk_response.h"
#include "../../globals.h"
#include "../../context.h"
#include "../../types.h"
#include "../../io.h"
#include "../../sw.h"
#include "../../crypto.h"
#include "../../common/buffer.h"
#include "../../common/bip32.h"

int handler_get_extended_public_key(buffer_t *cdata, bool has_access_token) {
    if (G_context.is_ui_busy) {
        return io_send_sw(SW_BUSY);
    }

    clear_context(&G_context, CMD_GET_EXTENDED_PUBLIC_KEY);

    cx_ecfp_private_key_t private_key = {0};
    cx_ecfp_public_key_t public_key = {0};

    uint32_t access_token;

    if (!buffer_read_u8(cdata, &G_context.ext_pub_ctx.bip32_path_len) ||
        !buffer_read_bip32_path(cdata, G_context.ext_pub_ctx.bip32_path, (size_t) G_context.ext_pub_ctx.bip32_path_len)
    ) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    if (has_access_token && !buffer_read_u32(cdata, &access_token, BE)) {
        return io_send_sw(SW_WRONG_DATA_LENGTH);
    }

    if (!bip32_path_validate(G_context.ext_pub_ctx.bip32_path,
                             G_context.ext_pub_ctx.bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_HARD_AT_LEAST_ACCOUNT)) {
        return io_send_sw(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    BEGIN_TRY {
        TRY {
            // derive private key according to BIP32 path
            crypto_derive_private_key(&private_key,
                                    G_context.ext_pub_ctx.chain_code,
                                    G_context.ext_pub_ctx.bip32_path,
                                    G_context.ext_pub_ctx.bip32_path_len);
            // generate corresponding public key
            crypto_init_public_key(&private_key,
                                   &public_key,
                                   G_context.ext_pub_ctx.raw_public_key);
        }
        CATCH_OTHER(e) {
            THROW(e);
        }
        FINALLY {
            // reset private key
            explicit_bzero(&private_key, sizeof(private_key));
        }
    }
    END_TRY;

    if (has_access_token && access_token == G_context.app_session_id) {
        return send_response_extended_pubkey();
    }

    return ui_display_account(access_token);
}