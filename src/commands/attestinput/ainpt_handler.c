//
//  autxo_handler.c
//  ErgoTxSerializer
//
//  Created by Yehor Popovych on 17.08.2021.
//

#include "ainpt_handler.h"
#include "ainpt_sw.h"
#include "ainpt_response.h"
#include "ainpt_ui.h"
#include "../../globals.h"
#include "../../helpers/session_id.h"
#include "../../helpers/response.h"

#include <string.h>

#define CHECK_SESSION(session_id) if (session_id != G_context.input_ctx.session) \
    return res_error(SW_ATTEST_UTXO_BAD_SESSION)

static inline int handle_init(buffer_t *cdata, bool has_token) {
    uint32_t prefix_size;
    uint32_t suffix_size;
    uint8_t tokens_count;
    uint32_t app_session_id = 0;

    clear_context(&G_context, CMD_ATTEST_INPUT_BOX);

    if (!buffer_read_u32(cdata, &prefix_size, BE)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u32(cdata, &suffix_size, BE)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u8(cdata, &tokens_count)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (has_token && !buffer_read_u32(cdata, &app_session_id, BE)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (buffer_can_read(cdata, 1)) {
        return res_error(SW_ATTEST_UTXO_MORE_DATA_THAN_NEEDED);
    }

    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_init(
        &G_context.input_ctx.tx,
        prefix_size,
        suffix_size,
        tokens_count,
        &G_context.input_ctx.tokens_table
    );
    
    if (res != ERGO_TX_SERIALIZER_SIMPLE_RES_OK) {
        return res_error(SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t)res);
    }
    
    G_context.input_ctx.session = session_id_new_random(G_context.input_ctx.session);

    if (has_token && G_context.app_session_id == app_session_id) {
        G_context.input_ctx.approved = true;
        return send_response_attested_input_session_id();
    }
    
    G_context.input_ctx.approved = false;
    return ui_display_access_token(app_session_id);
}

static inline int handle_tx_prefix_chunk(buffer_t *cdata) {
    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_add_prefix_chunk(
        &G_context.input_ctx.tx,
        cdata
    );
    
    if (res != ERGO_TX_SERIALIZER_SIMPLE_RES_OK &&
        res != ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA) {
        return res_error(SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t)res);
    }
    
    return res_ok();
}

static inline int handle_tx_tokens(buffer_t *cdata) {
    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_add_tokens(
        &G_context.input_ctx.tx,
        cdata
    );
    
    if (res != ERGO_TX_SERIALIZER_SIMPLE_RES_OK &&
        res != ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA) {
        return res_error(SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t)res);
    }
    
    return res_ok();
}

static inline int handle_tx_suffix_chunk(buffer_t *cdata) {
    ergo_tx_serializer_simple_result_e res = ergo_tx_serializer_simple_add_suffix_chunk(
        &G_context.input_ctx.tx,
        cdata
    );
    
    switch (res) {
        case ERGO_TX_SERIALIZER_SIMPLE_RES_OK:
            res = ergo_tx_serializer_simple_finalize(&G_context.input_ctx.tx,
                                                     G_context.input_ctx.tx_id);
            if (res != ERGO_TX_SERIALIZER_SIMPLE_RES_OK) {
                return res_error(SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t)res);
            }
            return res_ok();
        case ERGO_TX_SERIALIZER_SIMPLE_RES_MORE_DATA:
            return res_ok();
        default:
            return res_error(SW_ATTEST_UTXO_TX_ERROR_PREFIX + (uint8_t)res);
    }
}

static inline int handle_box_init(buffer_t *cdata) {
    uint16_t box_index;
    uint64_t value;
    uint32_t ergo_tree_size;
    uint32_t creation_height;
    uint8_t tokens_count;
    uint8_t registers_count;
    
    if (!buffer_read_u16(cdata, &box_index, BE)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u64(cdata, &value, BE)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u32(cdata, &ergo_tree_size, BE)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u32(cdata, &creation_height, BE)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u8(cdata, &tokens_count)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (!buffer_read_u8(cdata, &registers_count)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    if (buffer_can_read(cdata, 1)) {
        return res_error(SW_ATTEST_UTXO_MORE_DATA_THAN_NEEDED);
    }

    explicit_bzero(&G_context.input_ctx.box, sizeof(_attest_input_box_ctx_t));
    
    if (!ergo_tx_serializer_box_id_hash_init(&G_context.input_ctx.box.hash)) {
        return res_error(SW_ATTEST_UTXO_HASHER_ERROR);
    }
    
    ergo_tx_serializer_box_result_e res = ergo_tx_serializer_box_init(
        &G_context.input_ctx.box.ctx, value, ergo_tree_size,
        creation_height, tokens_count, registers_count,
        true, &G_context.input_ctx.tokens_table, &G_context.input_ctx.box.hash
    );
    
    if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
        return res_error(SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t)res);
    }
    
    G_context.input_ctx.box.box_index = box_index;
    G_context.input_ctx.box.tokens_count = tokens_count;
    
    return res_ok();
}

static inline int handle_box_tree_chunk(buffer_t *cdata) {
    ergo_tx_serializer_box_result_e res = ergo_tx_serializer_box_add_tree_chunk(
        &G_context.input_ctx.box.ctx, cdata
    );
    
    if (res != ERGO_TX_SERIALIZER_BOX_RES_OK &&
        res != ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA) {
        return res_error(SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t)res);
    }
    
    return res_ok();
}

static inline int handle_box_tokens(buffer_t *cdata) {
    ergo_tx_serializer_box_result_e res = ergo_tx_serializer_box_add_tokens(
        &G_context.input_ctx.box.ctx, cdata
    );
    
    if (res != ERGO_TX_SERIALIZER_BOX_RES_OK &&
        res != ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA) {
        return res_error(SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t)res);
    }
    
    return res_ok();
}

static inline int handle_box_register(buffer_t *cdata) {
    ergo_tx_serializer_box_result_e res = ergo_tx_serializer_box_add_register(
        &G_context.input_ctx.box.ctx, cdata
    );
    
    switch (res) {
        case ERGO_TX_SERIALIZER_BOX_RES_OK:
            res = ergo_tx_serializer_box_id_finalize(
                &G_context.input_ctx.box.hash,
                G_context.input_ctx.tx_id,
                G_context.input_ctx.box.box_index,
                G_context.input_ctx.box_id
            );
            if (res != ERGO_TX_SERIALIZER_BOX_RES_OK) {
                return res_error(SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t)res);
            }
            return send_response_attested_input_frame_count();
        case ERGO_TX_SERIALIZER_BOX_RES_MORE_DATA:
            return res_ok();
        default:
            return res_error(SW_ATTEST_UTXO_BOX_ERROR_PREFIX + (uint8_t)res);
    }
}

static inline int handle_box_get_frame(buffer_t *cdata) {
    uint8_t index;
    if (!buffer_read_u8(cdata, &index)) {
        return res_error(SW_ATTEST_UTXO_NOT_ENOUGH_PARAMS);
    }
    return send_response_attested_input_frame(index);
}

int handler_attest_input(buffer_t *cdata,
                         attest_input_subcommand_e subcommand,
                         uint8_t session_or_token) {
    if (G_context.is_ui_busy) {
        return res_ui_busy();
    }
    if (subcommand != ATTEST_INPUT_SUBCOMMAND_INIT &&
        !G_context.input_ctx.approved) {
        return res_deny();
    }
    switch (subcommand) {
        case ATTEST_INPUT_SUBCOMMAND_INIT:
            if (session_or_token != 0x01 && session_or_token != 0x02) {
                return res_error(SW_ATTEST_UTXO_BAD_P2);
            }
            return handle_init(cdata, session_or_token == 0x02);
        case ATTEST_INPUT_SUBCOMMAND_PREFIX_CHUNK:
            CHECK_SESSION(session_or_token);
            return handle_tx_prefix_chunk(cdata);
        case ATTEST_INPUT_SUBCOMMAND_TOKEN_IDS:
            CHECK_SESSION(session_or_token);
            return handle_tx_tokens(cdata);
        case ATTEST_INPUT_SUBCOMMAND_SUFFIX_CHUNK:
            CHECK_SESSION(session_or_token);
            return handle_tx_suffix_chunk(cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX:
            CHECK_SESSION(session_or_token);
            return handle_box_init(cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_TREE_CHUNK:
            CHECK_SESSION(session_or_token);
            return handle_box_tree_chunk(cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_TOKENS:
            CHECK_SESSION(session_or_token);
            return handle_box_tokens(cdata);
        case ATTEST_INPUT_SUBCOMMAND_BOX_REGISTER:
            CHECK_SESSION(session_or_token);
            return handle_box_register(cdata);
        case ATTEST_INPUT_SUBCOMMAND_GET_RESPONSE_FRAME:
            CHECK_SESSION(session_or_token);
            return handle_box_get_frame(cdata);
        default:
            return res_error(SW_ATTEST_UTXO_BAD_COMMAND);
    }
}
