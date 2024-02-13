#include <os.h>
#include <ux.h>
#include <glyphs.h>

#include "ainpt_ui.h"
#include "ainpt_response.h"
#include "../../context.h"
#include "../../common/macros_ext.h"
#include "../../helpers/response.h"
#include "../../ui/ui_application_id.h"
#include "../../ui/ui_approve_reject.h"
#include "../../ui/ui_menu.h"
#include "../../ui/ui_main.h"

// Step with icon and text
UX_STEP_NOCB(ux_ainpt_display_confirm_step, pn, {&C_icon_processing, "Confirm Attest Input"});

static NOINLINE void ui_action_attest_input(bool approved, void* context) {
    attest_input_ctx_t* ctx = (attest_input_ctx_t*) context;
    app_set_ui_busy(false);

    if (approved) {
        app_set_connected_app_id(ctx->ui.app_token_value);
        ctx->state = ATTEST_INPUT_STATE_APPROVED;
        send_response_attested_input_session_id(ctx->session);
    } else {
        app_set_current_command(CMD_NONE);
        res_deny();
    }

    ui_menu_main();
}

int ui_display_access_token(uint32_t app_access_token, attest_input_ctx_t* context) {
    context->ui.app_token_value = app_access_token;

    uint8_t screen = 0;
    ui_add_screen(&ux_ainpt_display_confirm_step, &screen);

    if (app_access_token != 0) {
        ui_add_screen(ui_application_id_screen(app_access_token, context->ui.app_token), &screen);
    }

    ui_approve_reject_screens(ui_action_attest_input, 
                              context, ui_next_sreen_ptr(&screen), ui_next_sreen_ptr(&screen));
    ui_display_screens(&screen);

    return 0;
}