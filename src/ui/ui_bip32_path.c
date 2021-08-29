#include "ui_bip32_path.h"
#include "../common/bip32.h"

static ux_layout_bnnn_paging_params_t G_ui_path_params[1] = {{.title = "Path", .text = NULL}};

const ux_flow_step_t ux_bip32_path_step = {
    ux_layout_bnnn_paging_init,
    G_ui_path_params,
    NULL,
    NULL,
};

const ux_flow_step_t* ui_bip32_path_screen(uint32_t* path,
                                           uint8_t path_len,
                                           char* buffer,
                                           uint8_t buffer_len) {
    G_ui_path_params->text = buffer;
    memset(buffer, 0, buffer_len);
    if (!bip32_path_format(path, path_len, buffer, buffer_len)) {
        return NULL;
    }
    return &ux_bip32_path_step;
}