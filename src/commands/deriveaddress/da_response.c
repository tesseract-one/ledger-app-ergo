#include <stddef.h>  // size_t
#include <stdint.h>  // uint*_t
#include <string.h>  // memmove

#include "da_response.h"
#include "../../sw.h"
#include "../../constants.h"
#include "../../context.h"
#include "../../globals.h"
#include "../../common/buffer.h"
#include "../../helpers/response.h"
#include "../../ergo/address.h"

int send_response_address(uint8_t address[static ADDRESS_LEN]) {
    BUFFER_NEW_LOCAL_EMPTY(response, ADDRESS_LEN);

    if (!buffer_write_bytes(&response, address, ADDRESS_LEN)) {
        return res_error(SW_BUFFER_ERROR);
    }

    clear_context(&G_context, CMD_NONE);

    return res_ok_data(&response);
}