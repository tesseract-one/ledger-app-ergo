#pragma once

#include <ux.h>
#include <stdint.h>

#define APPLICATION_ID_STR_LEN 11

const ux_flow_step_t* ui_application_id_screen(uint32_t app_id,
                                               char buffer[static APPLICATION_ID_STR_LEN]);