const { assert, expect } = require('chai')
    .use(require('chai-bytes'));
const { Transaction, TxId, Tokens, Token, TokenId, TokenAmount, I64 } = require('ergo-lib-wasm-nodejs');
const { toNetwork } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { UnsignedTransactionBuilder } = require('./helpers/transaction');

describe("Transaction Tests", function () {
    context("Transaction Commands", function () {
        it("can attest input", async function () {
            this.timeout(5000);
            const transaction = new UnsignedTransactionBuilder()
                .input(TEST_DATA.address0, TxId.zero(), 0)
                .build();
            const unsignedBox = transaction.inputs[0];
            await this.screenFlows.attestInput.do(
                () => this.device.attestInput(unsignedBox),
                attestedBox => {
                    expect(attestedBox).to.have.property('box');
                    expect(attestedBox.box).to.be.deep.equal(unsignedBox);
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
                },
                error => assert.fail(error)
            );
        });

        it("can sign tx", async function () {
            this.timeout(10000);
            const builder = new UnsignedTransactionBuilder()
                .input(TEST_DATA.address0, TxId.zero(), 0)
                .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress);
            const unsignedTransaction = builder.build();
            await this.screenFlows.signTx.do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                signatures => {
                    expect(signatures).to.have.length(1);
                    const unsigned = builder.buildErgo();
                    const signed = Transaction.from_unsigned_tx(unsigned, signatures);
                    // TODO verify signatures
                },
                error => assert.fail(error)
            );
        });

        it("can sign tx with zero data inputs", async function () {
            this.timeout(10000);
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(TEST_DATA.address0, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress)
                .build();
            await this.screenFlows.signTx.do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                signatures => {
                    expect(signatures).to.have.length(1);
                },
                error => assert.fail(error)
            );
        });

        it("can not sign tx with zero inputs", async function () {
            this.timeout(10000);
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress)
                .build();
            await this.screenFlows.signTx.do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                signatures => {
                    expect(signatures).to.be.empty;
                },
                error => assert.fail(error)
            );
        });

        it("can not sign tx with zero outputs", async function () {
            this.timeout(10000);
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(TEST_DATA.address0, TxId.zero(), 0)
                .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                .change(TEST_DATA.changeAddress)
                .build();
            await this.screenFlows.signTx.do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                signatures => expect(signatures).to.not.exist,
                error => {
                    expect(error).to.be.an('error');
                    expect(error.name).to.be.equal('DeviceError');
                    expect(error.message).to.be.equal('Bad output count');
                }
            );
        });

        it("can sign tx with tokens", async function () {
            this.timeout(10000);
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('0000000000000000000000000000000000000000000000000000000000000000');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(TEST_DATA.address0, TxId.zero(), 0, tokens)
                .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address, tokens)
                .change(TEST_DATA.changeAddress)
                .tokenId(tokenId.as_bytes())
                .build();
            await this.screenFlows.signTxWithTokens.do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                signatures => {
                    expect(signatures).to.have.length(1);
                },
                error => assert.fail(error)
            );
        });
    })
});
