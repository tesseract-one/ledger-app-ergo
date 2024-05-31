const { Address, NetworkPrefix, DerivationPath, ExtPubKey } = require("ergo-lib-wasm-nodejs");
const { toBytes } = require("./common");

class Account {
    constructor(publicKey, chainCode, path) {
        this.publicKey = ExtPubKey.new(toBytes(publicKey), toBytes(chainCode), path);
        this.path = path;
    }
}

class ExtendedAddress {
    constructor(network, address, path) {
        this.network = network;
        this.address = address;
        this.path = DerivationPath.new(path[0], [path[1]]);
        this.acc_index = path[0];
        this.addr_index = path[1];
    }

    toBase58() {
        return this.address.to_base58(this.network);
    }

    toBytes() {
        return this.address.to_bytes(this.network);
    }
}

class TestData {
    constructor(network) {
        this.network = network;
        this.account = new Account(
            '03c24e55008b523ccaf03b6c757f88c4881ef3331a255b76d2e078016c69c3dfd4',
            '8eb29c7897d57aee371bf254be6516e6963e2d9b379d0d626c17a39d1a3bf553',
            DerivationPath.from_string(`m/44'/429'/0'`)
        );
        this.address0 = new ExtendedAddress(
            network,
            Address.from_base58("9gqBSpseifxnkjRLZUxs5wbJGsvYPG7MLRcBgnKEzFiJoMJaakg"),
            [0, 0]
        );
        this.address1 = new ExtendedAddress(
            network,
            Address.from_base58("9iKPzGpzrEFFQ7kn2n6BHWU4fgTwSMF7atqPsvHAjgGvogSHz6Y"),
            [0, 1]
        );
        this.changeAddress = new ExtendedAddress(
            network,
            Address.from_base58("9eo8hALVQTaAuu8m95JhR8rhXuAMsLacaxM8X4omp724Smt8ior"),
            [0, 2]
        );
        this.changeAddress22 = new ExtendedAddress(
            network,
            Address.from_base58("9fRejXDJxxdJ1KVRH6HdxDj1S1duKmUNGG7CjztN2YjHnooxYAX"),
            [0, 22]
        );
    }
}

exports.TEST_DATA = new TestData(NetworkPrefix.Mainnet);
