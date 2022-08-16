const ergo = require('ergo-lib-wasm-nodejs');
const Buffer = require('buffer').Buffer;
const common = require('./common');
const { TEST_DATA } = require('./data');

function createErgoBox(recipient, txId, index, tokens = new ergo.Tokens()) {
    const ergoBox = new ergo.ErgoBox(
        ergo.BoxValue.from_i64(ergo.I64.from_str('1000000000')),
        0,
        ergo.Contract.pay_to_address(recipient),
        txId,
        index,
        tokens
    );
    return ergoBox;
}

function toUnsignedBox(ergoBox, contextExtension, signPath) {
    return {
        txId: ergoBox.tx_id().to_str(),
        index: ergoBox.index(),
        value: ergoBox.value().as_i64().to_str(),
        ergoTree: Buffer.from(ergoBox.ergo_tree().sigma_serialize_bytes()),
        creationHeight: ergoBox.creation_height(),
        tokens: common.toArray(ergoBox.tokens()).map(toToken),
        additionalRegisters: Buffer.from(ergoBox.serialized_additional_registers()),
        extension: Buffer.from(contextExtension.sigma_serialize_bytes()),
        signPath,
    };
}

function toToken(token) {
    return {
        id: token.id().to_str(),
        amount: token.amount().as_i64().to_str()
    };
}

function toBoxCandidate(output) {
    return {
        value: output.value().as_i64().to_str(),
        ergoTree: Buffer.from(output.ergo_tree().sigma_serialize_bytes()),
        creationHeight: output.creation_height(),
        tokens: common.toArray(output.tokens()).map(toToken),
        registers: Buffer.from(output.serialized_additional_registers()),
    };
}

class ErgoUnsignedTransactionBuilder {
    constructor() {
        this.amount = ergo.I64.from_str('0');
        this.inputs = ergo.ErgoBoxes.empty();
        this.dataInputs = new ergo.DataInputs();
        this.outputs = ergo.ErgoBoxCandidates.empty();
        this.feeAmount = ergo.TxBuilder.SUGGESTED_TX_FEE();
        this.changeAddress = null;
    }

    input(ergoBox) {
        this.inputs.add(ergoBox);
        return this;
    }

    dataInput(ergoBox) {
        this.dataInputs.add(new ergo.DataInput(ergoBox.box_id()));
        return this;
    }

    output(boxCandidate) {
        this.amount = this.amount.checked_add(boxCandidate.value().as_i64());
        this.outputs.add(boxCandidate);
        return this;
    }

    fee(amount) {
        this.feeAmount = amount;
    }

    change(address) {
        this.changeAddress = address;
        return this;
    }

    build() {
        const targetBalance = ergo.BoxValue.from_i64(this.amount.checked_add(this.feeAmount.as_i64()));
        const targetTokens = new ergo.Tokens();
        common.toArray(this.outputs)
            .flatMap(output => common.toArray(output.tokens()))
            .forEach(token => targetTokens.add(token));
        const boxSelection = new ergo.SimpleBoxSelector().select(this.inputs, targetBalance, targetTokens);
        const txBuilder = ergo.TxBuilder.new(
            boxSelection,
            this.outputs,
            0,
            this.feeAmount,
            this.changeAddress,
            ergo.BoxValue.SAFE_USER_MIN()
        );
        txBuilder.set_data_inputs(this.dataInputs);
        return txBuilder.build();
    }
}

class UnsignedTransactionBuilder {
    constructor() {
        this.network = TEST_DATA.network;
        this.ergoBuilder = new ErgoUnsignedTransactionBuilder();
        this.inputs = [];
        this.dataInputs = [];
        this.outputs = [];
        this.distinctTokenIds = [];
        this.changeMap = null;
    }

    input(extendedAddress, txId, index, tokens = new ergo.Tokens()) {
        const ergoBox = createErgoBox(extendedAddress.address, txId, index, tokens);
        const contextExtension = new ergo.ContextExtension();
        const unsignedBox = toUnsignedBox(ergoBox, contextExtension, extendedAddress.path.toString());
        this.inputs.push(unsignedBox);
        this.ergoBuilder.input(ergoBox);
        return this;
    }

    dataInput(address, txId, index) {
        const ergoBox = createErgoBox(address, txId, index);
        const dataInput = ergoBox.box_id().to_str();
        this.dataInputs.push(dataInput);
        this.ergoBuilder.dataInput(ergoBox);
        return this;
    }

    output(value, address, tokens = new ergo.Tokens()) {
        const builder = new ergo.ErgoBoxCandidateBuilder(
            ergo.BoxValue.from_i64(ergo.I64.from_str(value)),
            ergo.Contract.pay_to_address(address),
            0
        );
        common.toArray(tokens)
            .forEach(token => builder.add_token(token.id(), token.amount()));
        const output = builder.build();
        const boxCandidate = toBoxCandidate(output);
        this.outputs.push(boxCandidate);
        this.ergoBuilder.output(output);
        return this;
    }

    fee(value) {
        const amount = ergo.BoxValue.from_i64(ergo.I64.from_str(value));
        const builder = new ergo.ErgoBoxCandidateBuilder(
            amount,
            ergo.Contract.pay_to_address(common.getMinerAddress(this.network)),
            0
        );
        const output = builder.build();
        const boxCandidate = toBoxCandidate(output);
        this.outputs.push(boxCandidate);
        this.ergoBuilder.fee(amount);
        return this;
    }

    tokenIds(tokenIds) {
        this.distinctTokenIds = tokenIds;
        return this;
    }

    change(extendedAddress) {
        this.changeMap = {
            address: extendedAddress.toBase58(),
            path: extendedAddress.path.toString(),
        };
        this.ergoBuilder.change(extendedAddress.address);
        return this;
    }

    build() {
        return {
            inputs: this.inputs,
            dataInputs: this.dataInputs,
            outputs: this.outputs,
            distinctTokenIds: this.distinctTokenIds,
            changeMap: this.changeMap,
        };
    }

    buildErgo() {
        return this.ergoBuilder.build();
    }
}

exports.UnsignedTransactionBuilder = UnsignedTransactionBuilder;
