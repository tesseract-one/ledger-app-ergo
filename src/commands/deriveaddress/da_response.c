#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "da_response.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../globals.h"
#include "../../sw.h"
#include "../../common/buffer.h"

int send_response_address() {
    uint8_t resp[1 + PUBLIC_KEY_LEN] = {0};
    size_t offset = 0;

    resp[offset++] = 0x04;
    memmove(resp + offset, G_context.ext_pub_ctx.raw_public_key, PUBLIC_KEY_LEN);
    offset += PUBLIC_KEY_LEN;

    clear_context(&G_context, CMD_NONE);

    return io_send_response(&(const buffer_t){.ptr = resp, .size = offset, .offset = 0}, SW_OK);
}