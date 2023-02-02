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

#include <os.h>
#include <ux.h>

#include "glyphs.h"

#include "../globals.h"
#include "ui_menu.h"

UX_STEP_NOCB(ux_menu_ready_step, pnn, {&C_app_logo, APPNAME, "is ready"});
UX_STEP_CB(ux_menu_about_step, pb, ui_menu_about(), {&C_icon_certificate, "About"});
UX_STEP_CB(ux_menu_exit_step, pb, os_sched_exit(-1), {&C_icon_dashboard_x, "Quit"});

void ui_menu_main() {
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }

    G_ux_flow[0] = &ux_menu_ready_step;
    G_ux_flow[1] = &ux_menu_about_step;
    G_ux_flow[2] = &ux_menu_exit_step;
    G_ux_flow[3] = FLOW_LOOP;
    G_ux_flow[4] = FLOW_END_STEP;

    ux_flow_init(0, G_ux_flow, NULL);
}

UX_STEP_NOCB(ux_menu_info_step, bn, {APPNAME " App", "(c) 2021 Ergo"});
UX_STEP_NOCB(ux_menu_version_step, bn, {"Version", APPVERSION});
UX_STEP_CB(ux_menu_back_step, pb, ui_menu_main(), {&C_icon_back, "Back"});

void ui_menu_about() {
    G_ux_flow[0] = &ux_menu_info_step;
    G_ux_flow[1] = &ux_menu_version_step;
    G_ux_flow[2] = &ux_menu_back_step;
    G_ux_flow[3] = FLOW_LOOP;
    G_ux_flow[4] = FLOW_END_STEP;

    ux_flow_init(0, G_ux_flow, NULL);
}
