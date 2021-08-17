#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "da_response.h"
#include "da_sw.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../globals.h"
#include "../../common/buffer.h"

int send_response_address() {
    uint8_t _resp[PUBLIC_KEY_LEN] = {0};
    buffer_t response = {0};
    buffer_init(&response, _resp, PUBLIC_KEY_LEN, 0);

    if (!buffer_write_bytes(&response,
                            G_context.ext_pub_ctx.raw_public_key,
                            PUBLIC_KEY_LEN)) {
        clear_context(&G_context, CMD_NONE);
        return io_send_sw(SW_DERIVE_ADDRESS_BUFFER_ERR);
    }

    clear_context(&G_context, CMD_NONE);

    return io_send_response(&response, SW_OK);
}