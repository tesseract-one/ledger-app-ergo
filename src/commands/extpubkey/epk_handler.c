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
#include "../../helpers/crypto.h"
#include "../../helpers/response.h"
#include "../../common/buffer.h"
#include "../../common/bip32.h"
#include "../../helpers/session_id.h"

#define UI_CONTEXT(gctx) gctx.ui.ext_pub_key

int handler_get_extended_public_key(buffer_t *cdata, bool has_access_token) {
    if (G_context.ui.is_busy) {
        return res_ui_busy();
    }

    clear_context(&G_context, CMD_GET_EXTENDED_PUBLIC_KEY);

    cx_ecfp_private_key_t private_key = {0};
    cx_ecfp_public_key_t public_key = {0};

    uint8_t bip32_path_len;
    uint32_t bip32_path[MAX_BIP32_PATH];
    uint32_t access_token = 0;

    if (!buffer_read_u8(cdata, &bip32_path_len) ||
        !buffer_read_bip32_path(cdata, bip32_path, (size_t) bip32_path_len)) {
        return res_error(SW_WRONG_DATA_LENGTH);
    }

    if (has_access_token && !buffer_read_u32(cdata, &access_token, BE)) {
        return res_error(SW_WRONG_DATA_LENGTH);
    }

    if (!bip32_path_validate(bip32_path,
                             bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_ACCOUNT_GE3)) {
        return res_error(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    BEGIN_TRY {
        TRY {
            // derive private key according to BIP32 path
            crypto_derive_private_key(&private_key,
                                      UI_CONTEXT(G_context).chain_code,
                                      bip32_path,
                                      bip32_path_len);
            // generate corresponding public key
            crypto_init_public_key(&private_key, &public_key, UI_CONTEXT(G_context).raw_public_key);
        }
        CATCH_OTHER(e) {
            THROW(e);
        }
        FINALLY {
            // clear private key
            explicit_bzero(&private_key, sizeof(private_key));
        }
    }
    END_TRY;

    if (is_known_application(access_token, G_context.app_session_id)) {
        return send_response_extended_pubkey(UI_CONTEXT(G_context).raw_public_key,
                                             UI_CONTEXT(G_context).chain_code);
    }

    return ui_display_account(access_token,
                              bip32_path,
                              bip32_path_len,
                              UI_CONTEXT(G_context).raw_public_key,
                              UI_CONTEXT(G_context).chain_code);
}