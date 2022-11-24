#pragma once

#include <stdint.h>
#include <stdbool.h>

static inline bool network_id_is_mainnet(uint8_t network_id) {
    return network_id == 0x00;
}

static inline bool network_id_is_testnet(uint8_t network_id) {
    return network_id == 0x10;
}

static inline bool network_id_is_valid(uint8_t network_id) {
    return network_id <= 252;
}

static inline bool network_id_is_supported(uint8_t network_id) {
    return network_id_is_mainnet(network_id) || network_id_is_testnet(network_id);
}