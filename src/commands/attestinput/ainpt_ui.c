#include <os.h>
#include <ux.h>

#include "glyphs.h"

#include "ainpt_ui.h"
#include "ainpt_response.h"
#include "../../globals.h"
#include "../../common/macros.h"
#include "../../helpers/response.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_menu.h"

// Step with icon and text
UX_STEP_NOCB(ux_ainpt_display_confirm_step, pn, {&C_icon_processing, "Confirm Attest Input"});

static NOINLINE void ui_action_attest_input(bool approved, void* context) {
    G_context.is_ui_busy = false;
    attest_input_ctx_t* ctx = (attest_input_ctx_t*) context;

    if (approved) {
        G_context.app_session_id = ctx->ui.app_token_value;
        ctx->state = ATTEST_INPUT_STATE_APPROVED;
        send_response_attested_input_session_id(ctx->session);
    } else {
        res_deny();
    }

    ui_menu_main();
}

int ui_display_access_token(uint32_t app_access_token, attest_input_ctx_t* context) {
    context->ui.app_token_value = app_access_token;

    uint8_t screen = 0;
    G_ux_flow[screen++] = &ux_ainpt_display_confirm_step;
    if (app_access_token != 0) {
        G_ux_flow[screen++] = ui_application_id_screen(app_access_token, context->ui.app_token);
    }
    const ux_flow_step_t** approve = &G_ux_flow[screen++];
    const ux_flow_step_t** reject = &G_ux_flow[screen++];
    ui_approve_reject_screens(ui_action_attest_input, context, approve, reject);
    G_ux_flow[screen++] = FLOW_LOOP;
    G_ux_flow[screen++] = FLOW_END_STEP;

    ux_flow_init(0, G_ux_flow, NULL);

    G_context.is_ui_busy = true;

    return 0;
}