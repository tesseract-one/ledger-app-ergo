#include "ui_application_id.h"
#include <string.h>
#include <stdio.h>

static ux_layout_bn_params_t G_ui_application_id_params[1];

const ux_flow_step_t ux_app_token_step = {
    ux_layout_bn_init,
    G_ui_application_id_params,
    NULL,
    NULL,
};

const ux_flow_step_t* ui_application_id_screen(uint32_t app_id,
                                               char buffer[static APPLICATION_ID_STR_LEN]) {
    G_ui_application_id_params[0].line1 = "Application";
    G_ui_application_id_params[0].line2 = buffer;

    memset(buffer, 0, APPLICATION_ID_STR_LEN);
    snprintf(buffer, APPLICATION_ID_STR_LEN, "0x%08x", app_id);
    return &ux_app_token_step;
}