const Buffer = require("buffer").Buffer;
const b32path = require("bip32-path");
const bip32 = require("bip32");
const tx = require('./transaction');
const ergo = require('ergo-lib-wasm-nodejs');

const CLA = 0xE0;
const ERROR_CODES = {
    DENIED: 0x6985,
    WRONG_P1P2: 0x6A86,
    WRONG_APDU_DATA_LENGTH: 0x6A87,
    INS_NOT_SUPPORTED: 0x6D00,
    CLA_NOT_SUPPORTED: 0x6E00,
    BUSY: 0xB000,
    WRONG_RESPONSE_LENGTH: 0xB001,
    BAD_SESSION_ID: 0xB002,
    WRONG_SUBCOMMAND: 0xB003,
    BAD_STATE: 0xB0FF,
    BAD_TOKEN_ID: 0xE001,
    BAD_TOKEN_VALUE: 0xE002,
    BAD_CONTEXT_EXTENSION_SIZE: 0xE003,
    BAD_DATA_INPUT: 0xE004,
    BAD_BOX_ID: 0xE005,
    BAD_TOKEN_INDEX: 0xE006,
    BAD_FRAME_INDEX: 0xE007,
    BAD_INPUT_COUNT: 0xE008,
    BAD_OUTPUT_COUNT: 0xE009,
    TOO_MANY_TOKENS: 0xE00A,
    TOO_MANY_INPUTS: 0xE00B,
    TOO_MANY_DATA_INPUTS: 0xE00C,
    TOO_MANY_INPUT_FRAMES: 0xE00D,
    TOO_MANY_OUTPUTS: 0xE00E,
    HASHER_ERROR: 0xE00F,
    BUFFER_ERROR: 0xE010,
    U64_OVERFLOW: 0xE011,
    BIP32_BAD_PATH: 0xE012,
    INTERNAL_CRYPTO_ERROR: 0xE013,
    NOT_ENOUGH_DATA: 0xE014,
    TOO_MUCH_DATA: 0xE015,
    ADDRESS_GENERATION_FAILED: 0xE016,
    SCHNORR_SIGNING_FAILED: 0xE017,
    BAD_FRAME_SIGNATURE: 0xE018,
    BIP32_FORMATTING_FAILED: 0xE101,
    ADDRESS_FORMATTING_FAILED: 0xE102,
    STACK_OVERFLOW: 0xFFFF
};
const ERROR_MESSAGES = {
    [ERROR_CODES.DENIED]: "Operation denied by user",
    [ERROR_CODES.WRONG_P1P2]: "Incorrect P1 or P2",
    [ERROR_CODES.WRONG_APDU_DATA_LENGTH]: "Bad APDU length",
    [ERROR_CODES.INS_NOT_SUPPORTED]: "Instruction isn't supported",
    [ERROR_CODES.CLA_NOT_SUPPORTED]: "CLA is not supported",
    [ERROR_CODES.BUSY]: "Device is busy",
    [ERROR_CODES.WRONG_RESPONSE_LENGTH]: "Wrong response length",
    [ERROR_CODES.BAD_SESSION_ID]: "Bad session id",
    [ERROR_CODES.WRONG_SUBCOMMAND]: "Unknown subcommand",
    [ERROR_CODES.BAD_STATE]: "Bad state (check order of calls and errors)",
    [ERROR_CODES.BAD_TOKEN_ID]: "Bad token ID",
    [ERROR_CODES.BAD_TOKEN_VALUE]: "Bad token value",
    [ERROR_CODES.BAD_CONTEXT_EXTENSION_SIZE]: "Bad context extension size",
    [ERROR_CODES.BAD_DATA_INPUT]: "Bad data input ID",
    [ERROR_CODES.BAD_BOX_ID]: "Bad box ID",
    [ERROR_CODES.BAD_TOKEN_INDEX]: "Bad token index",
    [ERROR_CODES.BAD_FRAME_INDEX]: "Bad frame index",
    [ERROR_CODES.BAD_INPUT_COUNT]: "Bad input count",
    [ERROR_CODES.BAD_OUTPUT_COUNT]: "Bad output count",
    [ERROR_CODES.TOO_MANY_TOKENS]: "Too many tokens",
    [ERROR_CODES.TOO_MANY_INPUTS]: "Too many inputs",
    [ERROR_CODES.TOO_MANY_DATA_INPUTS]: "Too many data inputs",
    [ERROR_CODES.TOO_MANY_INPUT_FRAMES]: "Too many input frames",
    [ERROR_CODES.TOO_MANY_OUTPUTS]: "Too many outputs",
    [ERROR_CODES.HASHER_ERROR]: "Hasher internal error",
    [ERROR_CODES.BUFFER_ERROR]: "Buffer internal error",
    [ERROR_CODES.U64_OVERFLOW]: "UInt64 overflow",
    [ERROR_CODES.BIP32_BAD_PATH]: "Bad Bip32 path",
    [ERROR_CODES.INTERNAL_CRYPTO_ERROR]: "Internal crypto engine error",
    [ERROR_CODES.NOT_ENOUGH_DATA]: "Not enough data",
    [ERROR_CODES.TOO_MUCH_DATA]: "Too much data",
    [ERROR_CODES.ADDRESS_GENERATION_FAILED]: "Address generation failed",
    [ERROR_CODES.SCHNORR_SIGNING_FAILED]: "Schnorr signing failed",
    [ERROR_CODES.BAD_FRAME_SIGNATURE]: "Bad frame signature",
    [ERROR_CODES.BIP32_FORMATTING_FAILED]: "Can't display Bip32 path",
    [ERROR_CODES.ADDRESS_FORMATTING_FAILED]: "Can't display address",
    [ERROR_CODES.STACK_OVERFLOW]: "Stack overflow"
};

const COMMANDS = {
    app_version: 0x01,
    app_name: 0x02,
    extented_pub_key: 0x10,
    derive_address: 0x11,
    attest_input: 0x20,
    sign_tx: 0x21
}

class DeviceError extends Error {
    constructor(code) {
        const message = ERROR_MESSAGES[code] || "Unknown error";
        super(message);
        this.code = code;
    }
}

class Device {
    constructor(transport) {
        this.transport = transport;
        this.authToken = 0;
        this.generateNewAuthToken();
    }

    async command(code, p1, p2, data) {
        if (data.length > 255) {
            throw new DeviceError(ERROR_CODES.TOO_MUCH_DATA);
        }
        let header = Buffer.alloc(5);
        header.writeUInt8(CLA, 0);
        header.writeUInt8(code, 1);
        header.writeUInt8(p1, 2);
        header.writeUInt8(p2, 3);
        header.writeUInt8(data.length, 4);
        const response = await this.transport.exchange(Buffer.concat([header, data]))
        if (response.length < 2) {
            throw new DeviceError(ERROR_CODES.WRONG_RESPONSE_LENGTH);
        }
        const returnCode = response.readUInt16BE(response.length - 2);
        if (returnCode != 0x9000) {
            throw new DeviceError(returnCode); // Call error
        }
        return response.slice(0, response.length - 2);
    }

    async data(code, p1, p2, data) {
        let responses = []
        for (let i = 0; i < Math.ceil(data.length / 255); i++) {
            const chunk = data.slice(i * 255, Math.min((i + 1) * 255, data.length));
            const response = await this.command(code, p1, p2, chunk)
            responses.push(response)
        }
        return responses
    }

    generateNewAuthToken() {
        let newToken = 0;
        do {
            newToken = Math.floor(Math.random() * 0xFFFFFFFF) + 1;
        } while (newToken === this.authToken);
        this.authToken = newToken;
    }

    serializedToken() {
        const buf = Buffer.alloc(4);
        buf.writeUInt32BE(this.authToken);
        return buf;
    }

    getAppVersion() {
        return this.command(COMMANDS.app_version, 0x00, 0x00, Buffer.from([]))
    }

    getAppName() {
        return this.command(COMMANDS.app_name, 0x00, 0x00, Buffer.from([]))
            .then(buff => buff.toString('ascii'))
    }

    async getExtendedPubKey(account, useAuthToken) {
        const path = b32path.fromString(`m/44'/429'/${account}'`);
        const serPath = tx.serializeBip32Path(path);
        const message = useAuthToken
            ? Buffer.concat([serPath, this.serializedToken()])
            : serPath;
        const epk = await this.command(COMMANDS.extented_pub_key,
            useAuthToken ? 0x02 : 0x01, 0x00, message);
        const pk = epk.slice(0, 65);
        const cc = epk.slice(65);
        return bip32.fromPublicKey(pk, cc);
    }

    async attestInputSendHeader(box, useAuthToken) {
        let header = Buffer.alloc(0x37);
        let offset = 0;
        Buffer.from(box.tx_id().to_str(), 'hex').copy(header, offset);
        offset += 32;
        header.writeUInt16BE(box.index(), offset);
        offset += 2;
        Buffer.from(box.value().to_bytes()).copy(header, offset);
        offset += 8;
        header.writeUInt32BE(box.ergo_tree().sigma_serialize_bytes().length, offset);
        offset += 4;
        header.writeUInt32BE(box.creation_height(), offset);
        offset += 4;
        header.writeUInt8(box.tokens().len(), offset);
        offset += 1;
        header.writeUInt32BE(box.serialized_additional_registers().length, offset);
        offset += 4;
        if (useAuthToken) {
            header = Buffer.concat([header, this.serializedToken()])
        }
        const res = await this.command(COMMANDS.attest_input, 0x01,
            useAuthToken ? 0x02 : 0x01, header)
        return res[0];
    }

    async attestInputSendTree(data, sessionId) {
        const results = await this.data(COMMANDS.attest_input, 0x02, sessionId, data);
        return results.pop()[0];
    }

    async attestInputSendTokens(tokens, sessionId) {
        const packets = [];
        for (let i = 0; i < Math.ceil(tokens.len() / 6); i++) {
            const chunks = []
            for (let j = i * 6; j < Math.min((i + 1) * 6, tokens.len()); j++) {
                const token = tokens.get(j);
                const id = Buffer.from(token.id().as_bytes());
                const value = Buffer.from(token.amount().to_bytes());
                chunks.push(Buffer.concat([id, value]));
            }
            packets.push(Buffer.concat(chunks));
        }
        const results = [];
        for (let p of packets) {
            results.push(await this.command(COMMANDS.attest_input, 0x03, sessionId, p));
        }
        return results.pop()[0];
    }

    async attestInputSendRegisters(data, sessionId) {
        const results = await this.data(COMMANDS.attest_input, 0x04, sessionId, data);
        return results.pop()[0];
    }

    async getAttestedFrames(count, sessionId) {
        const frames = [];
        for (let i = 0; i < count; i++) {
            frames.push(await this.command(COMMANDS.attest_input, 0x05,
                sessionId, Buffer.from([i])));
        }
        return frames;
    }

    async attestInput(box, useAuthToken) {
        const sessionId = await this.attestInputSendHeader(box, useAuthToken);
        const tree = Buffer.from(box.ergo_tree().sigma_serialize_bytes());
        const tokens = box.tokens();
        const registers = Buffer.from(box.serialized_additional_registers());
        let frameCount = await this.attestInputSendTree(tree, sessionId);
        if (tokens.len() > 0) {
            frameCount = await this.attestInputSendTokens(tokens, sessionId);
        }
        if (registers.length > 0) {
            frameCount = await this.attestInputSendRegisters(registers, sessionId);
        }
        const frames = await this.getAttestedFrames(frameCount, sessionId);
        return new tx.AttestedBox(box, frames);
    }
}

exports.CLA = CLA;
exports.Device = Device;
exports.DeviceError = DeviceError;
exports.COMMANDS = COMMANDS;
exports.ERROR_CODES = ERROR_CODES;
exports.ERROR_MESSAGES = ERROR_MESSAGES;