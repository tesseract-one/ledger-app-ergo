#include <os.h>
#include <ux.h>

#include "ainpt_ui.h"
#include "ainpt_response.h"
#include "ainpt_sw.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../common/macros.h"
#include "../../helpers/response.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_menu.h"

#define CONTEXT(gctx)    gctx.ctx.attest_input
#define UI_CONTEXT(gctx) CONTEXT(gctx).ui

// Step with icon and text
UX_STEP_NOCB(ux_ainpt_display_confirm_step, pn, {&C_icon_processing, "Confirm Attest Input"});

int ui_display_access_token(uint32_t app_access_token) {
    UI_CONTEXT(G_context).app_token_value = app_access_token;

    uint8_t screen = 0;
    G_ux_flow[screen++] = &ux_ainpt_display_confirm_step;
    if (app_access_token != 0) {
        G_ux_flow[screen++] =
            ui_application_id_screen(app_access_token, UI_CONTEXT(G_context).app_token);
    }
    const ux_flow_step_t** approve = &G_ux_flow[screen++];
    const ux_flow_step_t** reject = &G_ux_flow[screen++];
    ui_approve_reject_screens(&ui_action_attest_input, approve, reject);
    G_ux_flow[screen++] = FLOW_LOOP;
    G_ux_flow[screen++] = FLOW_END_STEP;

    ux_flow_init(0, G_ux_flow, NULL);

    G_context.is_ui_busy = true;

    return 0;
}

void ui_action_attest_input(bool choice) {
    G_context.is_ui_busy = false;

    if (choice) {
        G_context.app_session_id = UI_CONTEXT(G_context).app_token_value;
        CONTEXT(G_context).state = ATTEST_INPUT_STATE_APPROVED;
        send_response_attested_input_session_id(CONTEXT(G_context).session);
    } else {
        res_deny();
    }

    ui_menu_main();
}