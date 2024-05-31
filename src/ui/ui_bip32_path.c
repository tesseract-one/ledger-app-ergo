#include "ui_bip32_path.h"
#include "../common/bip32_ext.h"

static ux_layout_bnnn_paging_params_t G_ui_path_params[1];
static ui_bip32_approve_callback G_ui_bip32_approve_callback;
static void* G_ui_bip32_approve_callback_context;

void ux_bip32_path_validate_init(unsigned int stack_slot) {
    UNUSED(stack_slot);
    if (G_ui_bip32_approve_callback != NULL) {
        G_ui_bip32_approve_callback(G_ui_bip32_approve_callback_context);
    } else {
        ux_flow_next();
    }
}

const ux_flow_step_t ux_bip32_path_validate_step = {ux_bip32_path_validate_init, NULL, NULL, NULL};

const ux_flow_step_t* const ux_bip32_path_validate[] = {&ux_bip32_path_validate_step,
                                                        FLOW_END_STEP};

const ux_flow_step_t ux_bip32_path_step = {
    ux_layout_bnnn_paging_init,
    G_ui_path_params,
    ux_bip32_path_validate,
    NULL,
};

const ux_flow_step_t* ui_bip32_path_screen(uint32_t* path,
                                           uint8_t path_len,
                                           const char* title,
                                           char* buffer,
                                           uint8_t buffer_len,
                                           ui_bip32_approve_callback cb,
                                           void* cb_context) {
    G_ui_path_params[0].title = title;
    G_ui_path_params[0].text = buffer;
    memset(buffer, 0, buffer_len);
    if (!bip32_path_format(path, path_len, buffer, buffer_len)) {
        return NULL;
    }
    G_ui_bip32_approve_callback = cb;
    G_ui_bip32_approve_callback_context = cb_context;
    return &ux_bip32_path_step;
}