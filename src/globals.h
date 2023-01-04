#pragma once

#include <stdint.h>

#include <ux.h>

#include "types.h"
#include "constants.h"
#include "context.h"
#include "helpers/io.h"

/**
 * Global buffer for interactions between SE and MCU.
 */
extern uint8_t G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

/**
 * Global structure to perform asynchronous UX aside IO operations.
 */
extern ux_state_t G_ux;

/**
 * Global structure with the parameters to exchange with the BOLOS UX application.
 */
extern bolos_ux_params_t G_ux_params;

/**
 * Global context for user requests.
 */
extern global_ctx_t G_context;

/**
 * Global array for UI screen flow
 */
extern ux_flow_step_t const *G_ux_flow[MAX_NUMBER_OF_SCREENS + 1];