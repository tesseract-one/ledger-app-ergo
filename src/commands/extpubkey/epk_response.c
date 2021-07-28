#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "epk_response.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../globals.h"
#include "../../sw.h"
#include "../../common/buffer.h"

int send_response_extended_pubkey() {
    uint8_t resp[EXTENDED_PUBLIC_KEY_LEN] = {0};
    size_t offset = 0;

    memmove(resp + offset, G_context.ext_pub_ctx.raw_public_key, PUBLIC_KEY_LEN);
    offset += PUBLIC_KEY_LEN;
    memmove(resp + offset, G_context.ext_pub_ctx.chain_code, CHAIN_CODE_LEN);
    offset += CHAIN_CODE_LEN;

    clear_context(&G_context, CMD_NONE);

    return io_send_response(&(const buffer_t){.ptr = resp, .size = offset, .offset = 0}, SW_OK);
}