const { NetworkPrefix } = require("ergo-lib-wasm-nodejs");
const { Network } = require("ledger-ergo-js");

async function sleep(ms) {
    await new Promise(r => setTimeout(r, ms != null ? ms : 500));
}

function toHex(bytes) {
    return Buffer.from(bytes).toString('hex');
}

function toBytes(hex) {
    return Uint8Array.from(Buffer.from(hex, 'hex'));
}

function toArray(object) {
    const array = [];
    for (let i = 0; i < object.len(); i++) {
        array.push(object.get(i));
    }
    return array;
}

function toNetwork(prefix) {
    switch (prefix) {
        case NetworkPrefix.Mainnet:
            return Network.Mainnet;
        case NetworkPrefix.Testnet:
            return Network.Testnet;
    }
}

function getApplication(device) {
    return '0x' + device.authToken.toString(16).padStart(8, 0);
}

function removeMasterNode(path) {
    return path.replace(/^(m\/)/, '');
}

exports.sleep = sleep;
exports.toHex = toHex;
exports.toBytes = toBytes;
exports.toArray = toArray;
exports.toNetwork = toNetwork;
exports.getApplication = getApplication;
exports.removeMasterNode = removeMasterNode;
