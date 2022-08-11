const { Address, NetworkPrefix, DerivationPath } = require("ergo-lib-wasm-nodejs");

class ExtendedAddress {
    constructor(network, address, path) {
        this.network = network;
        this.address = address;
        this.path = DerivationPath.new(path[0], [path[1]]);
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
        this.accountPath = DerivationPath.from_string(`m/44'/429'/0'`);
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
    }
}

exports.TEST_DATA = new TestData(NetworkPrefix.Mainnet);
