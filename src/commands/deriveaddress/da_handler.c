#include <stdint.h>   // uint*_t
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memset, explicit_bzero

#include <os.h>
#include <cx.h>

#include "da_handler.h"
#include "da_ui.h"
#include "da_response.h"
#include "da_sw.h"
#include "../../globals.h"
#include "../../context.h"
#include "../../types.h"
#include "../../helpers/crypto.h"
#include "../../common/buffer.h"
#include "../../common/bip32.h"
#include "../../helpers/response.h"
#include "../../helpers/session_id.h"

#define UI_CONTEXT(gcxt) G_context.ui.derive_address

static void genenerate_raw_public_key(uint32_t bip32_path[static MAX_BIP32_PATH],
                                      uint8_t path_len,
                                      uint8_t raw_key[static PUBLIC_KEY_LEN]) {
    cx_ecfp_private_key_t private_key = {0};
    cx_ecfp_public_key_t public_key = {0};
    uint8_t chain_code[CHAIN_CODE_LEN];

    BEGIN_TRY {
        TRY {
            // derive private key according to BIP32 path
            crypto_derive_private_key(&private_key, chain_code, bip32_path, path_len);
            // generate corresponding public key
            crypto_init_public_key(&private_key, &public_key, raw_key);
        }
        CATCH_OTHER(e) {
            THROW(e);
        }
        FINALLY {
            // reset private key and chaincode
            explicit_bzero(&private_key, sizeof(private_key));
            explicit_bzero(chain_code, CHAIN_CODE_LEN);
        }
    }
    END_TRY;
}

int handler_derive_address(buffer_t *cdata, bool display, bool has_access_token) {
    if (G_context.ui.is_busy) {
        return res_ui_busy();
    }

    clear_context(&G_context, CMD_DERIVE_ADDRESS);

    uint8_t bip32_path_len;
    uint32_t bip32_path[MAX_BIP32_PATH];

    uint32_t access_token = 0;
    uint8_t network_type = 0;

    if (!buffer_read_u8(cdata, &network_type)) {
        return res_error(SW_WRONG_DATA_LENGTH);
    }

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
                             BIP32_PATH_VALIDATE_ADDRESS_GE5)) {
        return res_error(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    genenerate_raw_public_key(bip32_path, bip32_path_len, UI_CONTEXT(G_context).raw_public_key);

    if (!display && is_known_application(access_token, G_context.app_session_id)) {
        return send_response_address(UI_CONTEXT(G_context).raw_public_key);
    }

    return ui_display_address(!display,
                              network_type,
                              access_token,
                              bip32_path,
                              bip32_path_len,
                              UI_CONTEXT(G_context).raw_public_key);
}