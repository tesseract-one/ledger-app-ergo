#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "epk_response.h"
#include "../../sw.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../common/rwbuffer.h"
#include "../../helpers/response.h"

#define WRITE_ERROR_HANDLER send_error
#include "../../helpers/cmd_macros.h"

static inline int send_error(uint16_t error) {
    app_set_current_command(CMD_NONE);
    return res_error(error);
}

int send_response_extended_pubkey(uint8_t raw_public_key[static PUBLIC_KEY_LEN],
                                  uint8_t chain_code[static CHAIN_CODE_LEN]) {
    RW_BUFFER_NEW_LOCAL_EMPTY(response, EXTENDED_PUBLIC_KEY_LEN);

    // Compressed pubkey
    CHECK_WRITE_PARAM(rw_buffer_write_u8(&response, ((raw_public_key[64] & 1) ? 0x03 : 0x02)));
    CHECK_WRITE_PARAM(rw_buffer_write_bytes(&response, raw_public_key + 1, 32));
    CHECK_WRITE_PARAM(rw_buffer_write_bytes(&response, chain_code, CHAIN_CODE_LEN));

    app_set_current_command(CMD_NONE);

    return res_ok_data(&response);
}