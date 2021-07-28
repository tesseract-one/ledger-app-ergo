#include <os.h>
#include <ux.h>

#include "da_ui.h"
#include "da_response.h"
#include "../../glyphs.h"
#include "../../globals.h"
#include "../../context.h"
#include "../../io.h"
#include "../../sw.h"
#include "../../ui.h"
#include "../../common/base58.h"
#include "../../common/macros.h"
#include "../../ergo/address.h"

static action_validate_cb g_da_validate_callback;

// Step with icon and text
UX_STEP_NOCB(ux_da_display_confirm_addr_step, pn, {&C_icon_eye, G_ui_ctx.derive_address.confirm_title});
// Step with title/text for account number
UX_STEP_NOCB(ux_da_display_path_step,
             bnnn_paging,
             {
                 .title = "Path",
                 .text = G_ui_ctx.derive_address.bip32_path,
             });
// Step with title/text for address
UX_STEP_NOCB(ux_da_display_address_step,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = G_ui_ctx.derive_address.address,
             });
// Step with title/text for application token
UX_STEP_NOCB(ux_da_display_app_token_step,
             bnnn_paging,
             {
                 .title = "Application",
                 .text = G_ui_ctx.derive_address.app_token,
             });
// Step with approve button
UX_STEP_CB(ux_da_display_approve_step,
           pb,
           (*g_da_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Approve",
           });
// Step with reject button
UX_STEP_CB(ux_da_display_reject_step,
           pb,
           (*g_da_validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

// FLOW to display address and BIP32 path:
// #1 screen: eye icon + "Confirm Address"
// #2 screen: display Bip32 path
// #3 screen: display address
// #4 screen: display application token
// #5 screen: approve button
// #6 screen: reject button
UX_FLOW(ux_da_display_address_flow,
        &ux_da_display_confirm_addr_step,
        &ux_da_display_path_step,
        &ux_da_display_address_step,
        &ux_da_display_app_token_step,
        &ux_da_display_approve_step,
        &ux_da_display_reject_step);

// Display
int ui_display_address(bool send, uint8_t network_id, uint32_t app_access_token) {
    if (G_context.current_command != CMD_DERIVE_ADDRESS) {
        return io_send_sw(SW_BAD_STATE);
    }

    if (!bip32_path_validate(G_context.derive_ctx.bip32_path,
                             G_context.derive_ctx.bip32_path_len,
                             BIP32_HARDENED(44),
                             BIP32_HARDENED(BIP32_ERGO_COIN),
                             BIP32_PATH_VALIDATE_AT_LEAST_ADDRESS)) {
        return io_send_sw(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    G_ui_ctx.derive_address.app_token_value = app_access_token;

    memset(G_ui_ctx.derive_address.bip32_path, 0, MEMBER_SIZE(derive_address_ui_ctx_t, bip32_path));
    if (!bip32_path_format(G_context.derive_ctx.bip32_path,
                           G_context.derive_ctx.bip32_path_len,
                           G_ui_ctx.derive_address.bip32_path,
                           MEMBER_SIZE(derive_address_ui_ctx_t, bip32_path))) {
        return io_send_sw(SW_DISPLAY_BIP32_PATH_FAIL);
    }

    memset(G_ui_ctx.derive_address.address, 0, MEMBER_SIZE(derive_address_ui_ctx_t, address));
    uint8_t address[ADDRESS_LEN] = {0};
    if (!address_from_pubkey(network_id, G_context.derive_ctx.raw_public_key, address, sizeof(address))) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }

    int result = base58_encode(address,
                               sizeof(address),
                               G_ui_ctx.derive_address.address,
                               sizeof(G_ui_ctx.derive_address.address));

    if (result == -1 || result >= ADDRESS_STRING_MAX_LEN) {
        return io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
    }

    memset(G_ui_ctx.derive_address.app_token, 0, MEMBER_SIZE(derive_address_ui_ctx_t, app_token));
    snprintf(G_ui_ctx.derive_address.app_token,
             MEMBER_SIZE(derive_address_ui_ctx_t, app_token),
             "0x%x",
             app_access_token);

    g_da_validate_callback = &ui_action_derive_address;

    if (send) {
        memcpy(G_ui_ctx.derive_address.confirm_title,
               "Confirm Send Address",
               sizeof("Confirm Send Address"));
    } else {
        memcpy(G_ui_ctx.derive_address.confirm_title,
               "Confirm Address",
               sizeof("Confirm Address"));
    }

    ux_flow_init(0, ux_da_display_address_flow, NULL);

    G_context.is_ui_busy = true;

    return 0;
}

// Action
void ui_action_derive_address(bool choice) {
    G_context.is_ui_busy = false;
    
    if (choice) {
        G_context.app_session_id = G_ui_ctx.derive_address.app_token_value;
        if (G_ui_ctx.derive_address.send) {
            send_response_address();
        } else {
            clear_context(&G_context, CMD_NONE);
            io_send_sw(SW_OK);
        }
    } else {
        clear_context(&G_context, CMD_NONE);
        io_send_sw(SW_DENY);
    }

    ui_menu_main();
}