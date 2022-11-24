#pragma once

#include <stdbool.h>
#include <ux.h>

typedef void (*ui_approve_reject_callback)(bool, void*);

void ui_approve_reject_screens(ui_approve_reject_callback cb,
                               void* context,
                               const ux_flow_step_t** approve,
                               const ux_flow_step_t** reject);