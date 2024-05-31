#include <stdint.h>
#include <stdbool.h>

#include <io.h>

#include "apdu_dispatcher.h"
#include "constants.h"
#include "sw.h"
#include "commands/app_name.h"
#include "commands/app_version.h"
#include "commands/extpubkey/epk_handler.h"
#include "commands/deriveaddress/da_handler.h"
#include "commands/attestinput/ainpt_handler.h"
#include "commands/signtx/stx_handler.h"

int apdu_dispatcher(const command_t *cmd) {
    if (cmd->cla != CLA) {
        return io_send_sw(SW_CLA_NOT_SUPPORTED);
    }

    buffer_t buf;
    buffer_init(&buf, cmd->data, cmd->lc);

    switch (cmd->ins) {
        case CMD_GET_APP_VERSION:
            if (cmd->p1 != 0 || cmd->p2 != 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            return handler_get_version();
        case CMD_GET_APP_NAME:
            if (cmd->p1 != 0 || cmd->p2 != 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            return handler_get_app_name();
        case CMD_GET_EXTENDED_PUBLIC_KEY:
            if (cmd->p1 == 0 || cmd->p1 > 2 || cmd->p2 > 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            return handler_get_extended_public_key(&buf, cmd->p1 == 2);
        case CMD_DERIVE_ADDRESS:
            if (cmd->p1 == 0 || cmd->p1 > 2 || cmd->p2 == 0 || cmd->p2 > 2) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            return handler_derive_address(&buf, cmd->p1 == 2, cmd->p2 == 2);
        case CMD_ATTEST_INPUT_BOX:
            if (cmd->p1 == 0 || cmd->p2 == 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            return handler_attest_input(&buf, cmd->p1, cmd->p2);
        case CMD_SIGN_TRANSACTION:
            if (cmd->p1 == 0 || cmd->p2 == 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            return handler_sign_transaction(&buf, cmd->p1, cmd->p2);
        default:
            return io_send_sw(SW_INS_NOT_SUPPORTED);
    }
}
