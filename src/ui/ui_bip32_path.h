#pragma once

#include <ux.h>
#include <stdint.h>

const ux_flow_step_t* ui_bip32_path_screen(uint32_t* path,
                                           uint8_t path_len,
                                           const char* title,
                                           char* buffer,
                                           uint8_t buffer_len);