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
#include "../../common/macros.h"
#include "../../helpers/response.h"
#include "../../helpers/session_id.h"

#define UI_CONTEXT(gcxt) G_context.ui.derive_address

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

    crypto_generate_public_key(bip32_path,
                               bip32_path_len,
                               UI_CONTEXT(G_context).raw_public_key,
                               NULL);

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