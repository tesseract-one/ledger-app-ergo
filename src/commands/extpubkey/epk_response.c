#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "epk_response.h"
#include "epk_sw.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../globals.h"
#include "../../common/buffer.h"
#include "../../helpers/response.h"

int send_response_extended_pubkey() {
    BUFFER_NEW_LOCAL_EMPTY(response, EXTENDED_PUBLIC_KEY_LEN);

    if (!buffer_write_bytes(&response,
                            G_context.ext_pub_ctx.raw_public_key,
                            PUBLIC_KEY_LEN)) {
        return res_error(SW_EXT_PUB_KEY_BUFFER_ERR);
    }

    if (!buffer_write_bytes(&response,
                            G_context.ext_pub_ctx.chain_code,
                            CHAIN_CODE_LEN)) {
        return res_error(SW_EXT_PUB_KEY_BUFFER_ERR);
    }

    clear_context(&G_context, CMD_NONE);

    return res_ok_data(&response);
}