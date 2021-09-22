#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "epk_response.h"
#include "../../sw.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../globals.h"
#include "../../common/buffer.h"
#include "../../helpers/response.h"

int send_response_extended_pubkey(uint8_t raw_public_key[static PUBLIC_KEY_LEN],
                                  uint8_t chain_code[static CHAIN_CODE_LEN]) {
    BUFFER_NEW_LOCAL_EMPTY(response, EXTENDED_PUBLIC_KEY_LEN);

    // Compressed pubkey
    if (!buffer_write_u8(&response, ((raw_public_key[64] & 1) ? 0x03 : 0x02))) {
        return false;
    }
    if (!buffer_write_bytes(&response, raw_public_key + 1, 32)) {
        return false;
    }

    if (!buffer_write_bytes(&response, chain_code, CHAIN_CODE_LEN)) {
        return res_error(SW_BUFFER_ERROR);
    }

    clear_context(&G_context, CMD_NONE);

    return res_ok_data(&response);
}