const chai = require('chai');
const { Transaction, TxId, verify_signature } = require('ergo-lib-wasm-nodejs');
const { Network } = require('ledger-ergo-js');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');
const { ADDRESS, ADDRESS2, CHANGE_ADDRESS, NETWORK } = require('./helpers/data');
const { UnsignedTransactionBuilder } = require('./helpers/transaction');

describe("Transaction Tests", function () {
    context("Transaction Commands", function () {
        it("can attest input", async function () {
            this.timeout(5000);
            const transaction = new UnsignedTransactionBuilder()
                .input(ADDRESS, TxId.zero(), 0, common.getAddressPath(0, 0))
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
                .input(ADDRESS, TxId.zero(), 0, common.getAddressPath(0, 0))
                .dataInput(ADDRESS, TxId.zero(), 0)
                .output('100000000', ADDRESS2)
                .change(CHANGE_ADDRESS, NETWORK, common.getAddressPath(0, 0, true));
            const unsignedTransaction = builder.build();
            const network = Network.Testnet;
            await this.screenFlows.signTx.do(
                () => this.device.signTx(unsignedTransaction, network),
                signatures => {
                    expect(signatures).to.exist;
                    const unsigned = builder.buildErgo();
                    const signed = Transaction.from_unsigned_tx(unsigned, signatures);
                    const verificationResult = verify_signature(
                        ADDRESS,
                        signed.sigma_serialize_bytes(),
                        signatures[0]
                    );
                    expect(verificationResult).to.be.equal(true);
                }
            );
        });
    })
});
