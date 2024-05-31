const ergo = require('ergo-lib-wasm-nodejs');
const Buffer = require('buffer').Buffer;
const common = require('./common');
const { TEST_DATA } = require('./data');

function createErgoBox(recipient, txId, index, amount, tokens) {
    const ergoBox = new ergo.ErgoBox(
        ergo.BoxValue.from_i64(ergo.I64.from_str(amount)),
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

function toDataInput(dataInput) {
    return dataInput.box_id().to_str();
}

function toToken(token) {
    return {
        id: token.id().to_str(),
        amount: token.amount().as_i64().to_str()
    };
}

function isTokensEqual(t1, t2) {
    return t1.id().to_str() === t2.id().to_str() &&
        t1.amount().as_i64().to_str() === t2.amount().as_i64().to_str();
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

class ErgoTxBuilder {
    constructor() {
        this.amount = ergo.I64.from_str('0');
        this.inputs = [];
        this.dataInputs = [];
        this.outputs = [];
        this.feeAmount = ergo.TxBuilder.SUGGESTED_TX_FEE();
        this.burningTokens = [];
        this.mintingOutputs = [];
        this.changeAddress = null;
        this.changeMap = null;
    }

    input(extendedAddress, txId, index, amount, tokens) {
        const ergoTokens = new ergo.Tokens();
        if (tokens) {
            tokens.forEach(t => {
                const token = new ergo.Token(
                    ergo.TokenId.from_str(t.id),
                    ergo.TokenAmount.from_i64(ergo.I64.from_str(t.amount))
                );
                ergoTokens.add(token);
            });
        }
        const eTxId = ergo.TxId.from_str(txId);
        const ergoBox = createErgoBox(extendedAddress.address, eTxId, index, amount, ergoTokens);
        return this.boxInput(ergoBox, extendedAddress.path);
    }

    boxInput(ergoBox, path) {
        const contextExtension = new ergo.ContextExtension();
        const box = toUnsignedBox(ergoBox, contextExtension, path.toString());
        this.inputs.push({box, ergo: ergoBox});
        return this;
    }

    dataInput(address, txId, index) {
        const eTxId = ergo.TxId.from_str(txId);
        const ergoBox = createErgoBox(
            address, eTxId, index, '100000000', new ergo.Tokens()
        );
        this.dataInputs.push(new ergo.DataInput(ergoBox.box_id()));
        return this;
    }

    output(address, value, send_tokens, mint_token_amount) {
        const builder = new ergo.ErgoBoxCandidateBuilder(
            ergo.BoxValue.from_i64(ergo.I64.from_str(value)),
            ergo.Contract.pay_to_address(address),
            0
        );
        if (send_tokens) {
            send_tokens.forEach(t => {
                builder.add_token(
                    ergo.TokenId.from_str(t.id),
                    ergo.TokenAmount.from_i64(ergo.I64.from_str(t.amount))
                );
            });
        }
        if (mint_token_amount) {
            const id = ergo.TokenId.from_str(this.inputs[0].ergo.box_id().to_str());
            const amount = ergo.TokenAmount.from_i64(ergo.I64.from_str(mint_token_amount));
            builder.add_token(id, amount);
            this.mintingOutputs.push(new ergo.Token(id, amount));
        } else {
            this.mintingOutputs.push(null);
        }
        const output = builder.build();
        this.amount = this.amount.checked_add(output.value().as_i64());
        this.outputs.push(output);
        return this;
    }

    burn(tokens) {
        const ergoTokens = tokens.map(t => new ergo.Token(
            ergo.TokenId.from_str(t.id),
            ergo.TokenAmount.from_i64(ergo.I64.from_str(t.amount))
        ));
        this.burningTokens.push(...ergoTokens);
        return this;
    }

    fee(amount) {
        this.feeAmount = amount ? ergo.BoxValue.from_i64(ergo.I64.from_str(amount)) : null;
        return this;
    }

    change(extendedAddress) {
        this.changeMap = {
            address: extendedAddress.toBase58(),
            path: extendedAddress.path.toString(),
        };
        this.changeAddress = extendedAddress.address;
        return this;
    }

    build() {
        const targetBalance = ergo.BoxValue.from_i64(this.amount.checked_add(this.feeAmount.as_i64()));

        const targetTokens = new ergo.Tokens();
        this.outputs.flatMap((output, idx) => {
            const minting = this.mintingOutputs[idx];
            return common.toArray(output.tokens()).filter(
                token => minting ? !isTokensEqual(token, minting) : true
            )
        }).forEach(token => targetTokens.add(token));
        this.burningTokens.forEach(token => targetTokens.add(token));

        const inputs = ergo.ErgoBoxes.empty();
        this.inputs.forEach(input => inputs.add(input.ergo));

        const outputs = ergo.ErgoBoxCandidates.empty();
        this.outputs.forEach(output => outputs.add(output));

        const dataInputs = new ergo.DataInputs();
        this.dataInputs.forEach(input => dataInputs.add(input));

        const boxSelection = new ergo.SimpleBoxSelector().select(inputs, targetBalance, targetTokens);

        const txBuilder = ergo.TxBuilder.new(
            boxSelection,
            outputs,
            0,
            this.feeAmount,
            this.changeAddress
        );
        if (this.burningTokens.length > 0) {
            const burn = new ergo.Tokens();
            this.burningTokens.forEach(token => burn.add(token));
            txBuilder.set_token_burn_permit(burn);
        }
        txBuilder.set_data_inputs(dataInputs);
        const ergoTx = txBuilder.build();

        const txInputs = common.toArray(ergoTx.inputs()).map(
            txinp => this.inputs.find(
                i => txinp.box_id().to_str() === i.ergo.box_id().to_str()
            )
        );
        
        const appTx = {
            inputs: txInputs.map(i => i.box),
            dataInputs: common.toArray(ergoTx.data_inputs()).map(toDataInput),
            outputs: common.toArray(ergoTx.output_candidates()).map(toBoxCandidate),
            distinctTokenIds: ergoTx.distinct_token_ids(),
            changeMap: this.changeMap,
        };
        return { appTx, ergoTx, uInputs: txInputs.map(i => i.ergo) };
    }
}

class TxBuilder extends ErgoTxBuilder {
    constructor(network = TEST_DATA.network) {
        super()
        this.network = network
    }

    buildAppTx() {
        const inputs = this.inputs.map(i => i.box);
        const outputs = this.outputs.map(toBoxCandidate);

        // add fee output
        if (this.feeAmount) {
            const feeBuilder = new ergo.ErgoBoxCandidateBuilder(
                this.feeAmount,
                ergo.Contract.pay_to_address(common.getMinerAddress(this.network)),
                0
            );
            outputs.push(toBoxCandidate(feeBuilder.build()));
        }

        // add change output
        if (this.changeAddress) {
            const sum = array => array
                .map(e => ergo.I64.from_str(e.value))
                .reduce((a, b) => a.checked_add(b), ergo.I64.from_str('0'));
            const amount = sum(inputs).as_num() - sum(outputs).as_num();

            const changeBuilder = new ergo.ErgoBoxCandidateBuilder(
                ergo.BoxValue.from_i64(ergo.I64.from_str(amount.toString())),
                ergo.Contract.pay_to_address(this.changeAddress.address),
                0
            );
            outputs.push(toBoxCandidate(changeBuilder.build()));
        }

        let distinctTokenIds = []
        outputs.forEach(output => {
            output.tokens.forEach(({id}) => {
                if (!distinctTokenIds.includes(id)) {
                    distinctTokenIds.push(id);
                }
            });
        });
        distinctTokenIds = distinctTokenIds.map(
            id => ergo.TokenId.from_str(id).as_bytes()
        );

        return {
            inputs: inputs,
            dataInputs: this.dataInputs.map(toDataInput),
            outputs: outputs,
            changeMap: this.changeMap,
            distinctTokenIds: distinctTokenIds
        };
    }
}

exports.TxBuilder = TxBuilder;