#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "ainpt_context.h"

/**
 * Display application access_token on the device and ask confirmation to proceed.
 *
 * @return 0 if success, negative integer otherwise.
 *
 */
int ui_display_access_token(uint32_t app_access_token, attest_input_ctx_t* context);