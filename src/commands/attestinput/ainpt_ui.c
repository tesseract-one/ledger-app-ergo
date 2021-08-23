#include <os.h>
#include <ux.h>

#include "ainpt_ui.h"
#include "ainpt_response.h"
#include "ainpt_sw.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../menu.h"
#include "../../common/macros.h"
#include "../../helpers/response.h"

#define UI_CONTEXT(gctx) gctx.ui.attest_inpt

// Step with icon and text
UX_STEP_NOCB(ux_ainpt_display_confirm_step, pn, {&C_icon_processing, "Confirm Attest Input"});
// Step with title/text for application token
UX_STEP_NOCB(ux_ainpt_display_app_token_step,
             bn,
             {
                 .line1 = "Application",
                 .line2 = UI_CONTEXT(G_context).app_token,
             });
// Step with approve button
UX_STEP_CB(ux_ainpt_display_approve_step,
           pb,
           ui_action_attest_input(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_ainpt_display_reject_step,
           pb,
           ui_action_attest_input(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display application token:
// #1 screen: eye icon + "Confirm Attest Input"
// #2 screen: display application token
// #3 screen: approve button
// #4 screen: reject button
UX_FLOW(ux_ainpt_display_confirm_flow,
        &ux_ainpt_display_confirm_step,
        &ux_ainpt_display_app_token_step,
        &ux_ainpt_display_approve_step,
        &ux_ainpt_display_reject_step,
        FLOW_LOOP);

int ui_display_access_token(uint32_t app_access_token) {
    UI_CONTEXT(G_context).app_token_value = app_access_token;

    memset(UI_CONTEXT(G_context).app_token, 0, MEMBER_SIZE(attest_input_ui_ctx_t, app_token));
    snprintf(UI_CONTEXT(G_context).app_token,
             MEMBER_SIZE(attest_input_ui_ctx_t, app_token),
             "0x%08x",
             app_access_token);

    ux_flow_init(0, ux_ainpt_display_confirm_flow, NULL);

    G_context.ui.is_busy = true;

    return 0;
}

void ui_action_attest_input(bool choice) {
    G_context.ui.is_busy = false;

    if (choice) {
        G_context.app_session_id = UI_CONTEXT(G_context).app_token_value;
        G_context.input_ctx.state = ATTEST_INPUT_STATE_APPROVED;
        send_response_attested_input_session_id(G_context.input_ctx.session);
    } else {
        res_deny();
    }

    ui_menu_main();
}