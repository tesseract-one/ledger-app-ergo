#include <stdint.h>  // uint*_t
#include <limits.h>  // UINT8_MAX
#include <assert.h>  // _Static_assert
#include "app_version.h"
#include "../constants.h"
#include "../helpers/response.h"
#include "../common/rwbuffer.h"

int handler_get_version() {
    _Static_assert(APPVERSION_LEN == 4, "Length of (MAJOR || MINOR || PATCH || DEBUG) must be 4!");
    _Static_assert(MAJOR_VERSION >= 0 && MAJOR_VERSION <= UINT8_MAX,
                   "MAJOR version must be between 0 and 255!");
    _Static_assert(MINOR_VERSION >= 0 && MINOR_VERSION <= UINT8_MAX,
                   "MINOR version must be between 0 and 255!");
    _Static_assert(PATCH_VERSION >= 0 && PATCH_VERSION <= UINT8_MAX,
                   "PATCH version must be between 0 and 255!");
    uint8_t version[APPVERSION_LEN] = {MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION, 0};
#ifdef DEBUG_BUILD
    version[APPVERSION_LEN - 1] = 1;
#endif

    RW_BUFFER_FROM_ARRAY_FULL(buf, version, APPVERSION_LEN);
    return res_ok_data(&buf);
}
