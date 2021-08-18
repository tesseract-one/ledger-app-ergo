/*****************************************************************************
 *   Ledger App Boilerplate.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "dispatcher.h"
#include "../constants.h"
#include "../globals.h"
#include "../types.h"
#include "../sw.h"
#include "../helpers/io.h"
#include "../common/buffer.h"
#include "../commands/app_name.h"
#include "../commands/app_version.h"
#include "../commands/extpubkey/epk_handler.h"
#include "../commands/deriveaddress/da_handler.h"
#include "../commands/attestinput/ainpt_handler.h"

int apdu_dispatcher(const command_t *cmd) {
    if (cmd->cla != CLA) {
        return io_send_sw(SW_CLA_NOT_SUPPORTED);
    }

    buffer_t buf;

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
            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }
            buffer_init(&buf, cmd->data, cmd->lc, cmd->lc);
            return handler_get_extended_public_key(&buf, cmd->p1 == 2);
        case CMD_DERIVE_ADDRESS:
            if (cmd->p1 == 0 || cmd->p1 > 2 || cmd->p2 == 0 || cmd->p2 > 2) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }
            buffer_init(&buf, cmd->data, cmd->lc, cmd->lc);
            return handler_derive_address(&buf, cmd->p1 == 2, cmd->p2 == 2);
        case CMD_ATTEST_INPUT_BOX:
            if (cmd->p1 == 0 || cmd->p2 == 0) {
                return io_send_sw(SW_WRONG_P1P2);
            }
            if (!cmd->data) {
                return io_send_sw(SW_WRONG_DATA_LENGTH);
            }
            buffer_init(&buf, cmd->data, cmd->lc, cmd->lc);
            return handler_attest_input(&buf, cmd->p1, cmd->p2);
        // case SIGN_TX:
        //     if ((cmd->p1 == P1_START && cmd->p2 != P2_MORE) ||  //
        //         cmd->p1 > P1_MAX ||                             //
        //         (cmd->p2 != P2_LAST && cmd->p2 != P2_MORE)) {
        //         return io_send_sw(SW_WRONG_P1P2);
        //     }

        //     if (!cmd->data) {
        //         return io_send_sw(SW_WRONG_DATA_LENGTH);
        //     }

        //     buf.ptr = cmd->data;
        //     buf.size = cmd->lc;
        //     buf.offset = 0;

        //     return handler_sign_tx(&buf, cmd->p1, (bool) (cmd->p2 & P2_MORE));
        default:
            return io_send_sw(SW_INS_NOT_SUPPORTED);
    }
}
