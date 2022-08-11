const { expect } = require('chai')
    .use(require('chai-bytes'))
    .use(require('chai-as-promised'));
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
                }
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
                    expect(signatures).to.exist;
                    const unsigned = builder.buildErgo();
                    const signed = Transaction.from_unsigned_tx(unsigned, signatures);
                    const verificationResult = verify_signature(
                        TEST_DATA.address0.address,
                        signed.sigma_serialize_bytes(),
                        signatures[0]
                    );
                    expect(verificationResult).to.be.equal(true);
                }
            );
        });

        it("can not sign tx with zero inputs", async function () {
            this.timeout(10000);
            const builder = new UnsignedTransactionBuilder()
                .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress);
            const unsignedTransaction = builder.build();
            await expect(this.screenFlows.signTx.do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                _ => { }
            )).to.be.rejectedWith(Error);
        });
    })
});
