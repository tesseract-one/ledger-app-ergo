#include <stdint.h>  // uint*_t

#include <os.h>

#include "app_name.h"
#include "../constants.h"
#include "../globals.h"
#include "../io.h"
#include "../sw.h"
#include "../types.h"
#include "common/buffer.h"

int handler_get_app_name() {
    _Static_assert(APPNAME_LEN < MAX_APPNAME_LEN, "APPNAME must be at most 64 characters!");

    buffer_t rdata = {.ptr = (uint8_t *) PIC(APPNAME), .size = APPNAME_LEN, .offset = 0};

    return io_send_response(&rdata, SW_OK);
}
