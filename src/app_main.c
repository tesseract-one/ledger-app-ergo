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

#include <stdint.h>  // uint*_t
#include <string.h>  // memset, explicit_bzero

#include <os.h>
#include <ux.h>
#include <cx.h>
#include <parser.h>
#include <io.h>

#include "context.h"
#include "sw.h"
#include "apdu_dispatcher.h"
#include "ui/ui_menu.h"
#include "common/macros_ext.h"

/**
 * Handle APDU command received and send back APDU response using handlers.
 */
void app_main() {
    // Length of APDU command received in G_io_apdu_buffer
    int input_len = 0;
    // Structured APDU command
    command_t cmd;

    // Initialize I/O
    io_init();

    // Initialize Global App Context
    app_init();

    // Show main menu
    ui_menu_main();

    for (;;) {
        // Receive command bytes in G_io_apdu_buffer
        if ((input_len = io_recv_command()) < 0) {
            PRINTF("=> io_recv_command failure\n");
            return;
        }

        // Parse APDU command from G_io_apdu_buffer
        if (!apdu_parser(&cmd, G_io_apdu_buffer, input_len)) {
            PRINTF("=> /!\\ BAD LENGTH: %.*H\n", input_len, G_io_apdu_buffer);
            io_send_sw(SW_WRONG_APDU_DATA_LENGTH);
            continue;
        }

        // Dispatch structured APDU command to handler
        if (apdu_dispatcher(&cmd) < 0) {
            PRINTF("=> apdu_dispatcher failure\n");
            return;
        }
    }
}
