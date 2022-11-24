#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../globals.h"

typedef uint16_t (*ui_dynamic_flow_show_screen_cb)(uint8_t, char *, char *, void *);

// Global context pointer will be set to the dynamic flow context. Don't change it.
bool ui_add_dynamic_flow_screens(uint8_t *screen,
                                 uint8_t dynamic_screen_count,
                                 char *title_storage,
                                 char *text_storage,
                                 ui_dynamic_flow_show_screen_cb show_cb,
                                 void *cb_ctx);