const { expect } = require('chai')
    .use(require('chai-bytes'));
const { Transaction, TxId, Tokens, Token, TokenId, TokenAmount, I64, ErgoBox } = require('ergo-lib-wasm-nodejs');
const { toNetwork, getApplication, removeMasterNode } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { AuthTokenFlows } = require('./helpers/flow');
const { UnsignedTransactionBuilder } = require('./helpers/transaction');

const signTxFlowCount = [5, 5];

function signTxFlows(device, auth, address, tokens = undefined) {
    const flows = [
        [
            { header: null, body: 'Confirm Attest Input' }
        ],
        [
            { header: null, body: 'Start P2PK signing' },
            { header: 'Path', body: removeMasterNode(address.path.toString()) }
        ],
        [
            { header: null, body: 'Confirm Transaction' },
            { header: 'P2PK Path', body: removeMasterNode(address.path.toString()) },
            { header: 'Transaction Amount', body: '0.100000000' },
            { header: 'Transaction Fee', body: '0.001000000' }
        ]
    ];
    if (tokens) {
        flows[2] = flows[2].concat(tokens);
    }
    if (auth) {
        flows[0].push({ header: 'Application', body: getApplication(device) });
        flows.splice(1, 1);
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
            const address = TEST_DATA.address0;
            const builder = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0)
                .dataInput(address.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .fee('1000000')
                .change(TEST_DATA.changeAddress);
            return { address, builder };
        }, signTxFlowCount).do(
            function () {
                const unsignedTransaction = this.builder.build();
                return this.test.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network))
            },
            function (signatures) {
                expect(this.flows).to.be.deep.equal(signTxFlows(this.test.device, this.auth, this.address));
                expect(signatures).to.have.length(1);
                const ergoBox = this.builder.ergoBuilder.inputs.get(0);
                verifySignatures(this.builder.ergoTransaction, signatures, ergoBox);
            }
        );

        new AuthTokenFlows("can sign tx with additional registers", () => {
            const address = TEST_DATA.address0;
            const ergoBox = ErgoBox.from_json(`{
                "boxId": "ef16f4a6db61a1c31aea55d3bf10e1fb6443cf08cff4a1cf2e3a4780e1312dba",
                "value": 1000000000,
                "ergoTree": "${address.address.to_ergo_tree().to_base16_bytes()}",
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
                .inputFrom(ergoBox, address.path)
                .output('100000000', TEST_DATA.address1.address)
                .fee('1000000')
                .change(TEST_DATA.changeAddress);
            return { address, builder };
        }, signTxFlowCount).do(
            function () {
                const unsignedTransaction = this.builder.build();
                return this.test.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network))
            },
            function (signatures) {
                expect(this.flows).to.be.deep.equal(signTxFlows(this.test.device, this.auth, this.address));
                expect(signatures).to.have.length(1);
                const ergoBox = this.builder.ergoBuilder.inputs.get(0);
                verifySignatures(this.builder.ergoTransaction, signatures, ergoBox);
            }
        );

        new AuthTokenFlows("can sign tx with zero data inputs", () => {
            const address = TEST_DATA.address0;
            const builder = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .fee('1000000')
                .change(TEST_DATA.changeAddress);
            return { address, builder };
        }, signTxFlowCount).do(
            function () {
                const unsignedTransaction = this.builder.build();
                return this.test.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                expect(this.flows).to.be.deep.equal(signTxFlows(this.test.device, this.auth, this.address));
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
        }, [2, 1]).do(
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
                        { header: null, body: 'Start P2PK signing' },
                        { header: 'Path', body: removeMasterNode(this.address.path.toString()) }
                    ]
                ];
                if (this.auth) {
                    signTxNoOutputsFlows[0].push({ header: 'Application', body: getApplication(this.test.device) });
                    signTxNoOutputsFlows.splice(1, 1);
                }
                expect(this.flows).to.be.deep.equal(signTxNoOutputsFlows);
                expect(error).to.be.an('error');
                expect(error.name).to.be.equal('DeviceError');
                expect(error.message).to.be.equal('Bad output count');
            }
        );

        new AuthTokenFlows("can sign tx with tokens", () => {
            const address = TEST_DATA.address0;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const builder = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0, tokens)
                .dataInput(address.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address, tokens)
                .fee('1000000')
                .change(TEST_DATA.changeAddress);
            return { address, builder };
        }, signTxFlowCount).do(
            function () {
                const unsignedTransaction = this.builder.build();
                return this.test.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                const tokens = [
                    { header: 'Token [1]', body: '29d2S7v...TmBhfV2' },
                    { header: 'Token [1]', body: 'T: 1000' }
                ];
                expect(this.flows).to.be.deep.equal(signTxFlows(this.test.device, this.auth, this.address, tokens));
                expect(signatures).to.have.length(1);
                const ergoBox = this.builder.ergoBuilder.inputs.get(0);
                verifySignatures(this.builder.ergoTransaction, signatures, ergoBox);
            }
        );

        new AuthTokenFlows("can sign tx with burned tokens", () => {
            const address = TEST_DATA.address0;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0, tokens)
                .dataInput(address.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .fee('1000000')
                .change(TEST_DATA.changeAddress)
                .tokenIds([tokenId.as_bytes()])
                .build(false);
            return { address, unsignedTransaction };
        }, signTxFlowCount).do(
            function () {
                return this.test.device.signTx(this.unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                const tokens = [
                    { header: 'Token [1]', body: '29d2S7v...TmBhfV2' },
                    { header: 'Token [1]', body: 'B: 1000; T: 0' }
                ];
                expect(this.flows).to.be.deep.equal(signTxFlows(this.test.device, this.auth, this.address, tokens));
                expect(signatures).to.have.length(1);
            }
        );

        new AuthTokenFlows("can sign tx with minted tokens", () => {
            const address = TEST_DATA.address0;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0)
                .dataInput(address.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address, tokens)
                .fee('1000000')
                .change(TEST_DATA.changeAddress)
                .tokenIds([tokenId.as_bytes()])
                .build(false);
            return { address, unsignedTransaction };
        }, signTxFlowCount).do(
            function () {
                return this.test.device.signTx(this.unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                const tokens = [
                    { header: 'Token [1]', body: '29d2S7v...TmBhfV2' },
                    { header: 'Token [1]', body: 'M: 1000; T: 1000' }
                ];
                expect(this.flows).to.be.deep.equal(signTxFlows(this.test.device, this.auth, this.address, tokens));
                expect(signatures).to.have.length(1);
            }
        );

        new AuthTokenFlows("can sign tx with few token ids", () => {
            const address = TEST_DATA.address0;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const tokens2 = new Tokens();
            const tokenId2 = TokenId.from_str('0000000000000000000000000000000000000000000000000000000000000000');
            tokens2.add(new Token(tokenId2, TokenAmount.from_i64(I64.from_str('1000'))));
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0, tokens)
                .dataInput(address.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address, tokens2)
                .fee('1000000')
                .change(TEST_DATA.changeAddress)
                .tokenIds([
                    tokenId.as_bytes(),
                    tokenId2.as_bytes()
                ])
                .build(false);
            return { address, unsignedTransaction };
        }, signTxFlowCount).do(
            function () {
                return this.test.device.signTx(this.unsignedTransaction, toNetwork(TEST_DATA.network));
            },
            function (signatures) {
                const tokens = [
                    { header: 'Token [1]', body: '29d2S7v...TmBhfV2' },
                    { header: 'Token [1]', body: 'B: 1000; T: 0' },
                    { header: 'Token [2]', body: '1111111...1111111' },
                    { header: 'Token [2]', body: 'M: 1000; T: 1000' }
                ];
                expect(this.flows).to.be.deep.equal(signTxFlows(this.test.device, this.auth, this.address, tokens));
                expect(signatures).to.have.length(1);
            }
        );
    });
});
