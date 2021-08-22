#include "stx_response.h"
#include "stx_sw.h"
#include "../../globals.h"
#include "../../helpers/response.h"

int send_response_sign_transaction_session_id(uint8_t session_id) {
    BUFFER_FROM_VAR_FULL(buf, session_id);
    return res_ok_data(&buf);
}
