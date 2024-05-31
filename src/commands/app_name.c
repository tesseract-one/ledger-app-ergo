#include <stdint.h>  // uint*_t

#include <os.h>

#include "app_name.h"
#include "../constants.h"
#include "../helpers/response.h"
#include "../common/rwbuffer.h"

int handler_get_app_name() {
    _Static_assert(APPNAME_LEN < MAX_APPNAME_LEN, "APPNAME must be at most 64 characters!");

    RW_BUFFER_FROM_ARRAY_FULL(buf, (uint8_t *) PIC(APPNAME), APPNAME_LEN);
    return res_ok_data(&buf);
}
