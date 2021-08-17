#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "epk_response.h"
#include "epk_sw.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../globals.h"
#include "../../common/buffer.h"

int send_response_extended_pubkey() {
    uint8_t _resp[EXTENDED_PUBLIC_KEY_LEN] = {0};
    buffer_t response = {0};
    buffer_init(&response, _resp, EXTENDED_PUBLIC_KEY_LEN, 0);

    if (!buffer_write_bytes(&response,
                            G_context.ext_pub_ctx.raw_public_key,
                            PUBLIC_KEY_LEN)) {
        clear_context(&G_context, CMD_NONE);
        return io_send_sw(SW_EXT_PUB_KEY_BUFFER_ERR);
    }

    if (!buffer_write_bytes(&response,
                            G_context.ext_pub_ctx.chain_code,
                            CHAIN_CODE_LEN)) {
        clear_context(&G_context, CMD_NONE);
        return io_send_sw(SW_EXT_PUB_KEY_BUFFER_ERR);
    }

    clear_context(&G_context, CMD_NONE);

    return io_send_response(&response, SW_OK);
}