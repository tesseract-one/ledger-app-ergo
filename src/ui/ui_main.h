#pragma once

#include <ux.h>
#include "../constants.h"
#include "../context.h"

#ifdef HAVE_BAGL
/**
 * Global array for UI screen flow
 */
extern ux_flow_step_t const* G_ux_flow_steps[MAX_NUMBER_OF_SCREENS + 1];

static inline const ux_flow_step_t** ui_next_sreen_ptr(uint8_t* position) {
    if (*position >= MAX_NUMBER_OF_SCREENS) return NULL;
    return &G_ux_flow_steps[(*position)++];
}

static inline bool ui_add_screen(const ux_flow_step_t* screen, uint8_t* position) {
    if (*position >= MAX_NUMBER_OF_SCREENS) return false;
    G_ux_flow_steps[(*position)++] = screen;
    return true;
}

static inline bool ui_display_screens(uint8_t* position) {
    if (MAX_NUMBER_OF_SCREENS - *position < 2) return false;

    G_ux_flow_steps[(*position)++] = FLOW_LOOP;
    G_ux_flow_steps[(*position)++] = FLOW_END_STEP;

    ux_flow_init(0, G_ux_flow_steps, NULL);

    app_set_ui_busy(true);

    return true;
}

#endif
