#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "da_response.h"
#include "da_sw.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../globals.h"
#include "../../common/buffer.h"
#include "../../helpers/response.h"

int send_response_address() {
    BUFFER_NEW_LOCAL_EMPTY(response, PUBLIC_KEY_LEN);
    
    if (!buffer_write_bytes(&response,
                            G_context.ext_pub_ctx.raw_public_key,
                            PUBLIC_KEY_LEN)) {
        return res_error(SW_DERIVE_ADDRESS_BUFFER_ERR);
    }

    clear_context(&G_context, CMD_NONE);

    return res_ok_data(&response);
}