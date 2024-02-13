#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "da_response.h"
#include "../../sw.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../common/rwbuffer.h"
#include "../../helpers/response.h"
#include "../../ergo/address.h"

#define WRITE_ERROR_HANDLER send_error
#include "../../helpers/cmd_macros.h"

static inline int send_error(uint16_t error) {
    app_set_current_command(CMD_NONE);
    return res_error(error);
}

int send_response_address(uint8_t address[static P2PK_ADDRESS_LEN]) {
    RW_BUFFER_NEW_LOCAL_EMPTY(response, P2PK_ADDRESS_LEN);

    CHECK_WRITE_PARAM(rw_buffer_write_bytes(&response, address, P2PK_ADDRESS_LEN));

    app_set_current_command(CMD_NONE);

    return res_ok_data(&response);
}