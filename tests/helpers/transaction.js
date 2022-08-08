const ergo = require('ergo-lib-wasm-nodejs');
const Buffer = require('buffer').Buffer;
const crypto = require('crypto');
const bip32 = require('bip32');
const b32path = require('bip32-path');

const minersFeeTree = Buffer.from([
    0x10, 0x05, 0x04, 0x00, 0x04, 0x00, 0x0e, 0x36, 0x10, 0x02, 0x04, 0xa0, 0x0b, 0x08, 0xcd,
    0x02, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87,
    0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16,
    0xf8, 0x17, 0x98, 0xea, 0x02, 0xd1, 0x92, 0xa3, 0x9a, 0x8c, 0xc7, 0xa7, 0x01, 0x73, 0x00,
    0x73, 0x01, 0x10, 0x01, 0x02, 0x04, 0x02, 0xd1, 0x96, 0x83, 0x03, 0x01, 0x93, 0xa3, 0x8c,
    0xc7, 0xb2, 0xa5, 0x73, 0x00, 0x00, 0x01, 0x93, 0xc2, 0xb2, 0xa5, 0x73, 0x01, 0x00, 0x74,
    0x73, 0x02, 0x73, 0x03, 0x83, 0x01, 0x08, 0xcd, 0xee, 0xac, 0x93, 0xb1, 0xa5, 0x73, 0x04
]);

function serializeBip32Path(path) {
    let arr = path.toPathArray();
    let out = Buffer.alloc(arr.length * 4 + 1);
    out.writeUInt8(path.length, 0);
    for (let i = 0; i < arr.length; i++) {
        out.writeUInt32BE(arr[i], i * 4 + 1);
    }
    return out;
}

class AttestedBox {
    constructor(box, frames) {
        this._box = box;
        this._frames = frames;
    }

    frames(extension) {
        const frames = this._frames;
        const ceSize = Buffer.alloc(4);
        ceSize.writeUInt32BE(extension.length);
        frames[0] = Buffer.concat([frames[0], ceSize]);
        return frames;
    }

    boxId() {
        return Buffer.from(this._box.box_id().as_bytes());
    }

    address() {
        return Buffer.from(ergo.Address.recreate_from_ergo_tree(this._box.ergo_tree()).to_bytes(1));
    }
}

class ExtendedOutput {
    constructor(output, pathes, tokens) {
        this._output = new ergo.ErgoBoxCandidate(); //output;
        this._pathes = pathes;
        this._tokens = [];//tokens;
    }

    header() {
        let value = Buffer.from(this._output.value().to_bytes());
        const buffer = Buffer.alloc(13);
        let offset = 0;
        if (this.isChangeOutput() || this.isMinersFeeOutput()) {
            buffer.writeUInt32BE(0, offset);
        } else {
            buffer.writeUInt32BE(this._ergoTree().length, offset);
        }
        offset += 4;
        buffer.writeUInt32BE(this._output.creation_height(), offset);
        offset += 4;
        buffer.writeUInt8(this._output.tokens().len());
        offset += 1;
        buffer.writeUInt32BE(this.registers().length, offset);
        return Buffer.concat([value, buffer]);
    }

    _ergoTree() {
        return Buffer.from(this._output.ergo_tree().sigma_serialize_bytes());
    }

    isMinersFeeOutput() {
        return this._ergoTree().equals(minersFeeTree);
    }

    isChangeOutput() {
        const address = this.address().toString('hex');
        return !!this._pathes[address];
    }

    address() {
        const address = ergo.Address.recreate_from_ergo_tree(this._output.ergo_tree());
        return Buffer.from(address.to_bytes(1));
    }

    ergoTree() {
        if (this.isMinersFeeOutput() || this.isChangeOutput()) {
            throw new Error("Wrong output type");
        }
        return this._ergoTree();
    }

    changeTree() {
        if (!this.isChangeOutput()) {
            throw new Error("Wrong output type");
        }
        const path = this._pathes[this.address().toString('hex')];
        return serializeBip32Path(path);
    }

    tokens() {
        const tokens = this._output.tokens();
        const out = Buffer.alloc(tokens.len() * 12);
        for (let i = 0; i < tokens.len(); i++) {
            const token = tokens.get(i);
            const tokenId = Buffer.from(token.id().as_bytes()).toString('hex');
            const index = this._tokens.indexOf(tokenId);
            out.writeUInt32BE(index, i * 12);
            Buffer.from(token.amount().to_bytes()).copy(out, i * 12 + 4);
        }
        return out;
    }

    registers() {
        return Buffer.from(this.output.register_value().sigma_serialize_bytes());
    }
}

class ExtendedTransaction {
    constructor(transaction, boxes, pathes) {
        //this.transaction = transaction;
        this._boxes = boxes;
        this._pathes = pathes;
        this._transaction = new ergo.UnsignedTransaction();
    }

    id() {
        return Buffer.from(this._transaction.id().to_str(), 'hex');
    }

    header() {
        const buffer = Buffer.alloc(7);
        let offset = 0;
        buffer.writeUInt16BE(this._transaction.inputs().len(), offset);
        offset += 2;
        buffer.writeUInt16BE(this._transaction.data_inputs().len(), offset);
        offset += 2;
        buffer.writeUInt8(this._transaction.distinct_token_ids().length, offset);
        offset += 1;
        buffer.writeUInt16BE(this._transaction.output_candidates().len(), offset);
        return buffer;
    }

    tokenIds() {
        const ids = this._transaction.distinct_token_ids();
        const packets = [];
        for (let i = 0; i < Math.ceil(ids.length / 7); i++) {
            const buffer = Buffer.concat(ids.slice(i * 7, Math.min((i + 1) * 7, ids.length)));
            packets.push(buffer);
        }
        return packets;
    }

    inputs() {
        const inputs = this._transaction.inputs();
        const result = [];
        for (let i = 0; i < inputs.len(); i++) {
            const input = inputs.get(i);
            const boxId = Buffer.from(input.box_id().as_bytes()).toString('hex');
            const box = this._boxes[boxId];
            if (!box) { throw new Error("Box not found"); }
            const extension = Buffer.from(input.extension().sigma_serialize_bytes());
            const frames = box.frames(extension);
            result.push({ frames, extension, box })
        }
        return result;
    }

    dataInputs() {
        const inputs = this._transaction.data_inputs();
        const packets = [];
        for (let i = 0; i < Math.ceil(inputs.len() / 7); i++) {
            const chunks = []
            for (let j = i * 7; j < Math.min((i + 1) * 7, inputs.len()); j++) {
                chunks.push(Buffer.from(inputs.get(j).box_id().as_bytes()));
            }
            const buffer = Buffer.concat(chunks);
            packets.push(buffer);
        }
        return packets;
    }

    outputs() {
        const tokens = this.distinct_token_ids().map((id) => Buffer.from(id).toString('hex'));
        const outputs = this._transaction.output_candidates();
        const mapped = [];
        for (let i = 0; i < outputs.len(); i++) {
            mapped.push(new ExtendedOutput(outputs.get(i), this._pathes, tokens));
        }
        return mapped;
    }

    pathes() {
        return this.inputs().map((inpt) => {
            const box = inpt.box;
            const path = this._pathes[box.boxId().toString('hex')];
            if (!path) { throw new Error("Path not found"); }
            const frame = serializeBip32Path(path);
            return { box, path, frame };
        });
    }
}

class TransactionGenerator {
    constructor(account, path, height) {
        this.account = bip32.fromBase58("");
        this.height = height;
        this.input_boxes = [];
        this.output_boxes = [];
        this.change_boxes = [];
        this.pathes = {};
        this.path = path;
    }

    createToken(value) {
        const rawId = crypto.randomBytes(32);
        const id = ergo.TokenId.from_str(rawId.toString('hex'));
        const i64 = typeof value === "string"
            ? ergo.I64.from_str(value)
            : ergo.I64.from_str(value.toString(10));
        const val = ergo.TokenAmount.from_i64(i64);
        return new ergo.Token(id, val);
    }

    changeTokenValue(token, value) {
        const i64 = typeof value === "string"
            ? ergo.I64.from_str(value)
            : ergo.I64.from_str(value.toString(10));
        const val = ergo.TokenAmount.from_i64(i64);
        return new ergo.Token(token.id(), val);
    }

    addInput(value, change, addrIndex, tokens) {
        const pComp = [change ? 1 : 0, addrIndex];
        const rawId = crypto.randomBytes(32);
        const tx_id = ergo.TxId.from_str(rawId.toString('hex'));
        const height = Math.floor(Math.random() * this.height);
        const pubKey = this.account.derive(pComp[0]).derive(pComp[1]);
        const address = ergo.Address.from_public_key(pubKey.publicKey);
        const path = b32path.fromPathArray(this.path.toPathArray() + pComp);
        this.pathes[Buffer.from(address.to_bytes(1)).toString('hex')] = path;
        const contract = ergo.Contract.pay_to_address(address);
        const index = Math.floor(Math.random() * 0xFFFF);
        const tns = new ergo.Tokens();
        for (let token of tokens) {
            tns.add(token);
        }
        const input = new ergo.ErgoBox(value, height, contract, tx_id, index, tns);
        this.input_boxes.push(input);
    }

    getInputBoxes() {
        return this.input_boxes;
    }

    getTxData(attestedInputs) {
        const boxes = attestedInputs.reduce((dict, box) => {
            const id = box.boxId().toString('hex');
            dict[id] = box;
            return dict;
        }, {});

        const boxSel = new ergo.BoxSelection()
        const txBuilder = ergo.TxBuilder.new()

        const transaction = new ergo.UnsignedTransaction();
        return new ExtendedTransaction(transaction, boxes, this.pathes);
    }
}

function createUnsignedTransaction() {
    const inputs = new ergo.UnsignedInputs();
    const boxId = ergo.BoxId.from_str("0000000000000000000000000000000000000000000000000000000000000000");
    const ext = new ergo.ContextExtension();
    inputs.add(new ergo.UnsignedInput(boxId, ext));
    const dataInputs = new ergo.DataInputs();
    dataInputs.add(new ergo.DataInput(boxId));
    const feeAmount = ergo.BoxValue.from_i64(ergo.I64.from_str("1000000"));
    const creationHeight = 1;
    const boxCandidate = ergo.ErgoBoxCandidate.new_miner_fee_box(feeAmount, creationHeight);
    const outputCandidates = new ergo.ErgoBoxCandidates(boxCandidate);
    const transaction = new ergo.UnsignedTransaction(inputs, dataInputs, outputCandidates);
    return transaction;
}

exports.AttestedBox = AttestedBox;
exports.ExtendedTransaction = ExtendedTransaction;
exports.ExtendedOutput = ExtendedOutput;
exports.TransactionGenerator = TransactionGenerator;
exports.serializeBip32Path = serializeBip32Path;
exports.createUnsignedTransaction = createUnsignedTransaction;