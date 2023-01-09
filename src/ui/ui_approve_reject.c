#include "ui_approve_reject.h"
#include "../glyphs.h"

static ui_approve_reject_callback G_ui_approve_reject_callback;
static void* G_ui_approve_reject_callback_context;

// Step with approve button
UX_STEP_CB(ux_approve_step,
           pb,
           G_ui_approve_reject_callback(true, G_ui_approve_reject_callback_context),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_reject_step,
           pb,
           G_ui_approve_reject_callback(false, G_ui_approve_reject_callback_context),
           {
               &C_icon_crossmark,
               "Reject",
           });

void ui_approve_reject_screens(ui_approve_reject_callback cb,
                               void* context,
                               const ux_flow_step_t** approve,
                               const ux_flow_step_t** reject) {
    G_ui_approve_reject_callback = cb;
    G_ui_approve_reject_callback_context = context;
    *approve = &ux_approve_step;
    *reject = &ux_reject_step;
}