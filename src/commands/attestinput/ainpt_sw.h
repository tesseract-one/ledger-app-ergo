#pragma once

#include "../../sw.h"
#include "../../ergo/tx_ser_simple.h"
#include "../../ergo/tx_ser_box.h"

static inline uint16_t sw_from_tx_res(ergo_tx_serializer_simple_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_SIMPLE_RES_OK:
            return SW_OK;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA:
            return SW_OK;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_TOKEN_ID:
            return SW_BAD_TOKEN_ID;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_PREFIX_LEN:
            return SW_BAD_TRANSACTION_PREFIX_LEN;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_SUFFIX_LEN:
            return SW_BAD_TRANSACTION_SUFFIX_LEN;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_TOO_MANY_TOKENS:
            return SW_TOO_MANY_TOKENS;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_TOO_MUCH_DATA:
            return SW_TOO_MUCH_DATA;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_HASHER:
            return SW_HASHER_ERROR;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BUFFER:
            return SW_BUFFER_ERROR;
        case ERGO_TX_SERIALIZER_SIMPLE_RES_ERR_BAD_STATE:
            return SW_BAD_STATE;
    }
}

static inline uint16_t sw_from_box_res(ergo_tx_serializer_box_result_e res) {
    switch (res) {
        case ERGO_TX_SERIALIZER_BOX_RES_OK:
            return SW_OK;
        case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:
            return SW_OK;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_INDEX:
            return SW_BAD_TOKEN_INDEX;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_BAD_TOKEN_VALUE:
            return SW_BAD_TOKEN_VALUE;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_TOKENS:
            return SW_TOO_MANY_TOKENS;
        case ERGO_TX_SERIALIZER_BOX_RES_ERR_TOO_MANY_REGISTERS:
            return SW_TOO_MANY_REGISTERS;
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
    }
}
