#pragma once

#include "../sw.h"
#include "../ergo/tx_ser_box.h"
#include "../ergo/tx_ser_full.h"

static inline uint16_t sw_from_tx_full_result(ergo_tx_serializer_full_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_FULL_RES_OK:
            return SW_OK;
        case ERGO_TX_SERIALIZER_FULL_RES_MORE_DATA:
            return SW_OK;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_ID:
            return SW_BAD_TOKEN_ID;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_VALUE:
            return SW_BAD_TOKEN_VALUE;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_CONTEXT_EXTENSION_SIZE:
            return SW_BAD_CONTEXT_EXTENSION_SIZE;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_DATA_INPUT:
            return SW_BAD_DATA_INPUT;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_INPUT_ID:
            return SW_BAD_BOX_ID;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_TOKEN_INDEX:
            return SW_BAD_TOKEN_INDEX;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_FRAME_INDEX:
            return SW_BAD_FRAME_INDEX;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_INPUT_COUNT:
            return SW_BAD_INPUT_COUNT;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_OUTPUT_COUNT:
            return SW_BAD_OUTPUT_COUNT;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_TOKENS:
            return SW_TOO_MANY_TOKENS;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_INPUTS:
            return SW_TOO_MANY_INPUTS;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_DATA_INPUTS:
            return SW_TOO_MANY_DATA_INPUTS;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_INPUT_FRAMES:
            return SW_TOO_MANY_INPUT_FRAMES;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MANY_OUTPUTS:
            return SW_TOO_MANY_OUTPUTS;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_TOO_MUCH_DATA:
            return SW_TOO_MUCH_DATA;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_HASHER:
            return SW_HASHER_ERROR;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BUFFER:
            return SW_BUFFER_ERROR;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_BAD_STATE:
            return SW_BAD_STATE;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_U64_OVERFLOW:
            return SW_U64_OVERFLOW;
        case ERGO_TX_SERIALIZER_FULL_RES_ERR_SMALL_CHUNK:
            return SW_SMALL_CHUNK;
    }
}

static inline uint16_t sw_from_tx_box_result(ergo_tx_serializer_box_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_BOX_RES_OK:
            return SW_OK;
        case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:
            return SW_OK;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX:
            return SW_BAD_TOKEN_INDEX;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_ID:
            return SW_BAD_TOKEN_ID;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE:
            return SW_BAD_TOKEN_VALUE;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS:
            return SW_TOO_MANY_TOKENS;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MUCH_DATA:
            return SW_TOO_MUCH_DATA;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_HASHER:
            return SW_HASHER_ERROR;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BUFFER:
            return SW_BUFFER_ERROR;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_STATE:
            return SW_BAD_STATE;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_U64_OVERFLOW:
            return SW_U64_OVERFLOW;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_SMALL_CHUNK:
            return SW_SMALL_CHUNK;
    }
}
