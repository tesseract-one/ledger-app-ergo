const { Address, NetworkPrefix } = require("ergo-lib-wasm-nodejs");

const NETWORK = NetworkPrefix.Testnet;
const ADDRESS = Address.from_base58("3WxvL2bGTBLAP2wiBmoFbsTPHE6Mu5bjxQPh8xKq6eK6p6Y89qAj");
const ADDRESS2 = Address.from_base58("3WzQYa3DoJtSqgL3dF6Nv52G3ctuJ8gsiervM9To2PjfS6s68W6E");
const CHANGE_ADDRESS = Address.from_base58("3Wy8RUojcXSBKQsvbLfbH4dPMx5JyevAVceTL4bkUWmHX2sQoPsw");

exports.NETWORK = NETWORK;
exports.ADDRESS = ADDRESS;
exports.ADDRESS2 = ADDRESS2;
exports.CHANGE_ADDRESS = CHANGE_ADDRESS;
