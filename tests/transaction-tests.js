const { expect } = require('chai')
    .use(require('chai-bytes'));
const { assert } = require('chai');
const { Transaction, TxId } = require('ergo-lib-wasm-nodejs');
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
            const builder = new UnsignedTransactionBuilder()
                .input(TEST_DATA.address0, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress);
            const unsignedTransaction = builder.build();
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
            const builder = new UnsignedTransactionBuilder()
                .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress);
            const unsignedTransaction = builder.build();
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
            const builder = new UnsignedTransactionBuilder()
                .input(TEST_DATA.address0, TxId.zero(), 0)
                .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                .change(TEST_DATA.changeAddress);
            const unsignedTransaction = builder.build();
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
    })
});
