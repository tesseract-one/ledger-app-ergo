const Buffer = require("buffer").Buffer;

const CLA = 0xE0;

const ERROR_CODES = {
    0x6985: "Operation denied by user",
    0x6A86: "Incorrect P1 or P2",
    0x6A87: "Bad APDU length",
    0x6D00: "Instruction isn't supported",
    0x6E00: "CLA is not supported",
    0xB000: "Device is busy",
    0xB001: "Wrong response length",
    0xB002: "Bad session id",
    0xB003: "Unknown subcommand",
    0xB0FF: "Bad state (check order of calls and errors)",
    0xE001: "Bad token ID",
    0xE002: "Bad token value",
    0xE003: "Bad context extension size",
    0xE004: "Bad data input ID",
    0xE005: "Bad box ID",
    0xE006: "Bad token index",
    0xE007: "Bad frame index",
    0xE008: "Bad input count",
    0xE009: "Bad output count",
    0xE00A: "Too many tokens",
    0xE00B: "Too many inputs",
    0xE00C: "Too many data inputs",
    0xE00D: "Too many input frames",
    0xE00E: "Too many outputs",
    0xE00F: "Hasher internal error",
    0xE010: "Buffer internal error",
    0xE011: "UInt64 overflow",
    0xE012: "Bad Bip32 path",
    0xE013: "Internal crypto engine error",
    0xE014: "Not enough data",
    0xE015: "Too much data",
    0xE016: "Address generation failed",
    0xE017: "Schnorr signing failed",
    0xE018: "Bad frame signature",
    0xE101: "Can't display Bip32 path",
    0xE102: "Can't display address",
    0xFFFF: "Stack overflow"
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
        const message = ERROR_CODES[code] || "Unknown error";
        super(message);
        this.code = code;
    }
}

class Device {
    constructor(transport) {
        this.transport = transport;
    }

    async command(code, p1, p2, data) {
        if (data.length > 255) {
            throw new DeviceError(0xE015); // Too much data
        }
        let header = Buffer.alloc(5);
        header.writeUInt8(CLA, 0);
        header.writeUInt8(code, 1);
        header.writeUInt8(p1, 2);
        header.writeUInt8(p2, 3);
        header.writeUInt8(data.length, 4);
        const response = await this.transport.exchange(Buffer.concat([header, data]))
        if (response.length < 2) {
            throw new DeviceError(0xB001); // Wrong response length
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

    hasAutomation() {
        return !!this.speculos.automationSocket;
    }

    getAppVersion() {
        return this.command(COMMANDS.app_version, 0x00, 0x00, Buffer.from([]))
    }

    getAppName() {
        return this.command(COMMANDS.app_name, 0x00, 0x00, Buffer.from([]))
            .then(buff => buff.toString('ascii'))
    }
}

exports.CLA = CLA;
exports.Device = Device;
exports.DeviceError = DeviceError;
exports.COMMANDS = COMMANDS;
exports.ERROR_CODES = ERROR_CODES;