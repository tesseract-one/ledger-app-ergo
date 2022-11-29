const { expect } = require('chai')
    .use(require('chai-bytes'));
const { Transaction, TxId, Tokens, Token, TokenId, TokenAmount, I64, ErgoBox } = require('ergo-lib-wasm-nodejs');
const { toNetwork, getApplication, removeMasterNode, ellipsize } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { AuthTokenFlows } = require('./helpers/flow');
const { UnsignedTransactionBuilder } = require('./helpers/transaction');

const signTxFlowCount = [5, 5];

function signTxFlows(device, auth, from, to, change, tokens = undefined) {
    const flows = [
        [
            { header: null, body: 'Confirm Attest Input' }
        ],
        [
            { header: 'P2PK Signing', body: removeMasterNode(from.path.toString()) },
            { header: 'Application', body: '0x00000000' },
            { header: null, body: 'Approve' },
            { header: null, body: 'Reject' }
        ],
        [
            { header: null, body: 'Confirm Output' },
            { header: 'Address', body: ellipsize(to.toBase58()) },
            { header: 'Output Value', body: '0.100000000' }
        ],
        [
            { header: null, body: 'Confirm Output' },
            { header: 'Change', body: removeMasterNode(change.path.toString()) }
        ],
        [
            { header: null, body: 'Approve Signing' },
            { header: 'P2PK Path', body: removeMasterNode(from.path.toString()) },
            { header: 'Transaction Amount', body: '0.100000000' },
            { header: 'Transaction Fee', body: '0.001000000' }
        ]
    ];
    if (tokens) {
        flows[2] = flows[2].concat(tokens);
    }
    if (auth) {
        flows[0].push({ header: 'Application', body: getApplication(device) });
        flows[1].splice(1, 1);
    }
    return flows;
};

function verifySignatures(unsigned, signatures, ergoBox) {
    const signed = Transaction.from_unsigned_tx(unsigned, signatures);
    const verificationResult = signed.verify_p2pk_input(ergoBox);
    expect(verificationResult).to.be.true;
}

describe("Transaction Tests", function () {
    context("Transaction Commands", function () {
        new AuthTokenFlows("can attest input", () => {
            return {
                unsignedBox: new UnsignedTransactionBuilder()
                    .input(TEST_DATA.address0, TxId.zero(), 0)
                    .output('100000000', TEST_DATA.address1.address)
                    .change(TEST_DATA.changeAddress)
                    .build()
                    .inputs[0]
            };
        }).do(
            function () {
                return this.test.device.attestInput(this.unsignedBox);
            },
            function (attestedBox) {
                const attestInputFlow = [{ header: null, body: 'Confirm Attest Input' }];
                if (this.auth) {
                    attestInputFlow.push({ header: 'Application', body: getApplication(this.test.device) });
                }
                expect(this.flows[0]).to.be.deep.equal(attestInputFlow);
                expect(attestedBox).to.have.property('box');
                expect(attestedBox.box).to.be.deep.equal(this.unsignedBox);
                expect(attestedBox).to.have.property('frames');
                expect(attestedBox.frames).to.have.length(1);
                const frame = attestedBox.frames[0];
                expect(frame.boxId).to.exist;
                expect(frame.framesCount).to.be.equal(1);
                expect(frame.frameIndex).to.be.equal(0);
                expect(frame.amount).to.be.equal('1000000000');
                expect(frame.tokens).to.be.empty;
                expect(frame.attestation).to.exist;
                expect(frame.buffer).to.exist;
            }
        );

        new AuthTokenFlows("can sign tx", () => {
            const from = TEST_DATA.address0;
            const to = TEST_DATA.address1;
            const change = TEST_DATA.changeAddress;
            const builder = new UnsignedTransactionBuilder()
                .input(from, TxId.zero(), 0)
                .dataInput(from.address, TxId.zero(), 0)
                .output('100000000', to.address)
                .fee('1000000')
                .change(change);
            return { from, to, change, builder };
        }, signTxFlowCount).do(
            function () {
                const unsignedTransaction = this.builder.build();
                return this.test.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network))
            },
            function (signatures) {
                let flows = signTxFlows(this.test.device, this.auth, this.from, this.to, this.change);
                expect(this.flows).to.be.deep.equal(flows);
                expect(signatures).to.have.length(1);
                const ergoBox = this.builder.ergoBuilder.inputs.get(0);
                verifySignatures(this.builder.ergoTransaction, signatures, ergoBox);
            }
        );

        new AuthTokenFlows("can sign tx with additional registers", () => {
            const from = TEST_DATA.address0;
            const to = TEST_DATA.address1;
            const change = TEST_DATA.changeAddress;
            const ergoBox = ErgoBox.from_json(`{
                "boxId": "ef16f4a6db61a1c31aea55d3bf10e1fb6443cf08cff4a1cf2e3a4780e1312dba",
                "value": 1000000000,
                "ergoTree": "${from.address.to_ergo_tree().to_base16_bytes()}",
                "assets": [],
                "additionalRegisters": {
                  "R5": "0e050102030405",
                  "R4": "04f601"
                },
                "creationHeight": 0,
                "transactionId": "0000000000000000000000000000000000000000000000000000000000000000",
                "index": 0
            }`);
            const builder = new UnsignedTransactionBuilder()
                .inputFrom(ergoBox, from.path)
                .output('100000000', to.address)
                .fee('1000000')
                .change(change);
            return { from, to, change, builder };
        }, signTxFlowCount).do(
            function () {
                const unsignedTransaction = this.builder.build();
                return this.test.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network))
            },
            function (signatures) {
                let flows = signTxFlows(this.test.device, this.auth, this.from, this.to, this.change);
                expect(this.flows).to.be.deep.equal(flows);
                expect(signatures).to.have.length(1);
                const ergoBox = this.builder.ergoBuilder.inputs.get(0);
                verifySignatures(this.builder.ergoTransaction, signatures, ergoBox);
            }
        );

        new AuthTokenFlows("can sign tx with zero data inputs", () => {
            const from = TEST_DATA.address0;
            const to = TEST_DATA.address1;
            const change = TEST_DATA.changeAddress;
            const builder = new UnsignedTransactionBuilder()
                .input(from, TxId.zero(), 0)
                .output('100000000', to.address)
                .fee('1000000')
                .change(change);
            return { from, to, change, builder };
        }, signTxFlowCount).do(
            function () {
                const unsignedTransaction = this.builder.build();
                return this.test.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                let flows = signTxFlows(this.test.device, this.auth, this.from, this.to, this.change);
                expect(this.flows).to.be.deep.equal(flows);
                expect(signatures).to.have.length(1);
                const ergoBox = this.builder.ergoBuilder.inputs.get(0);
                verifySignatures(this.builder.ergoTransaction, signatures, ergoBox);
            }
        );

        new AuthTokenFlows("can not sign tx with zero inputs", () => {
            return {
                unsignedTransaction: new UnsignedTransactionBuilder()
                    .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                    .output('100000000', TEST_DATA.address1.address)
                    .build(false)
            };
        }, [0, 0]).do(
            function () {
                return this.test.device.signTx(this.unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                expect(this.flows).to.be.empty;
                expect(signatures).to.be.empty;
            }
        );

        new AuthTokenFlows("can not sign tx with zero outputs", () => {
            const address = TEST_DATA.address0;
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0)
                .dataInput(address.address, TxId.zero(), 0)
                .build(false);
            return { address, unsignedTransaction };
        }, [2, 2]).do(
            function () {
                return this.test.device.signTx(this.unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                expect(signatures).to.not.exist;
            }, function (error) {
                const signTxNoOutputsFlows = [
                    [
                        { header: null, body: 'Confirm Attest Input' }
                    ],
                    [
                        { header: 'P2PK Signing', body: removeMasterNode(this.address.path.toString()) },
                        { header: 'Application', body: '0x00000000' },
                        { header: null, body: 'Approve' },
                        { header: null, body: 'Reject' }
                    ]
                ];
                if (this.auth) {
                    signTxNoOutputsFlows[0].push({ header: 'Application', body: getApplication(this.test.device) });
                    signTxNoOutputsFlows[1].splice(1, 1);
                }
                expect(this.flows).to.be.deep.equal(signTxNoOutputsFlows);
                expect(error).to.be.an('error');
                expect(error.name).to.be.equal('DeviceError');
                expect(error.message).to.be.equal('Bad output count');
            }
        );

        new AuthTokenFlows("can sign tx with tokens", () => {
            const from = TEST_DATA.address0;
            const to = TEST_DATA.address1;
            const change = TEST_DATA.changeAddress;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const builder = new UnsignedTransactionBuilder()
                .input(from, TxId.zero(), 0, tokens)
                .dataInput(from.address, TxId.zero(), 0)
                .output('100000000', to.address, tokens)
                .fee('1000000')
                .change(change);
            return { from, to, change, builder };
        }, signTxFlowCount).do(
            function () {
                const unsignedTransaction = this.builder.build();
                return this.test.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                const tokens = [
                    { header: 'Token [1]', body: '1111111...1111111' },
                    { header: 'Token [1] Value', body: '1000' }
                ];
                let flows = signTxFlows(this.test.device, this.auth, this.from, this.to, this.change, tokens);
                expect(this.flows).to.be.deep.equal(flows);
                expect(signatures).to.have.length(1);
                const ergoBox = this.builder.ergoBuilder.inputs.get(0);
                verifySignatures(this.builder.ergoTransaction, signatures, ergoBox);
            }
        );

        new AuthTokenFlows("can sign tx with burned tokens", () => {
            const from = TEST_DATA.address0;
            const to = TEST_DATA.address1;
            const change = TEST_DATA.changeAddress;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(from, TxId.zero(), 0, tokens)
                .dataInput(from.address, TxId.zero(), 0)
                .output('100000000', to.address)
                .fee('1000000')
                .change(change)
                .tokenIds([tokenId.as_bytes()])
                .build(false);
            return { from, to, change, unsignedTransaction };
        }, signTxFlowCount).do(
            function () {
                return this.test.device.signTx(this.unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                const tokens = [
                    { header: 'Token [1]', body: '1111111...1111111' },
                    { header: 'Token [1] Value', body: 'Burning: 1000' }
                ];
                let flows = signTxFlows(this.test.device, this.auth, this.from, this.to, this.change);
                flows[4] = flows[4].concat(tokens);
                expect(this.flows).to.be.deep.equal(flows);
                expect(signatures).to.have.length(1);
            }
        );

        new AuthTokenFlows("can sign tx with minted tokens", () => {
            const from = TEST_DATA.address0;
            const to = TEST_DATA.address1;
            const change = TEST_DATA.changeAddress;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(from, TxId.zero(), 0)
                .dataInput(from.address, TxId.zero(), 0)
                .output('100000000', to.address, tokens)
                .fee('1000000')
                .change(change)
                .tokenIds([tokenId.as_bytes()])
                .build(false);
            return { from, to, change, unsignedTransaction };
        }, signTxFlowCount).do(
            function () {
                return this.test.device.signTx(this.unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                const tokens = [
                    { header: 'Token [1]', body: '1111111...1111111' },
                    { header: 'Token [1] Value', body: '1000' }
                ];
                let flows = signTxFlows(this.test.device, this.auth, this.from, this.to, this.change, tokens);
                flows[4] = flows[4].concat([
                    { header: 'Token [1]', body: '1111111...1111111' },
                    { header: 'Token [1] Value', body: 'Minting: 1000' }
                ]);
                expect(this.flows).to.be.deep.equal(flows);
                expect(signatures).to.have.length(1);
            }
        );

        new AuthTokenFlows("can sign tx with few token ids", () => {
            const from = TEST_DATA.address0;
            const to = TEST_DATA.address1;
            const change = TEST_DATA.changeAddress;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const tokens2 = new Tokens();
            const tokenId2 = TokenId.from_str('0000000000000000000000000000000000000000000000000000000000000000');
            tokens2.add(new Token(tokenId2, TokenAmount.from_i64(I64.from_str('1000'))));
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(from, TxId.zero(), 0, tokens)
                .dataInput(from.address, TxId.zero(), 0)
                .output('100000000', to.address, tokens2)
                .fee('1000000')
                .change(change)
                .tokenIds([
                    tokenId.as_bytes(),
                    tokenId2.as_bytes()
                ])
                .build(false);
            return { from, to, change, unsignedTransaction };
        }, signTxFlowCount).do(
            function () {
                return this.test.device.signTx(this.unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                const tokens = [
                    { header: 'Token [1]', body: '0000000...0000000' },
                    { header: 'Token [1] Value', body: '1000' }
                ];
                let flows = signTxFlows(this.test.device, this.auth, this.from, this.to, this.change, tokens);
                flows[4] = flows[4].concat([
                    { header: 'Token [1]', body: '1111111...1111111' },
                    { header: 'Token [1] Value', body: 'Burning: 1000' },
                    { header: 'Token [2]', body: '0000000...0000000' },
                    { header: 'Token [2] Value', body: 'Minting: 1000' }
                ]);
                expect(this.flows).to.be.deep.equal(flows);
                expect(signatures).to.have.length(1);
            }
        );
    });
});
