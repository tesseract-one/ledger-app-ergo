const chai = require('chai');
const { Network } = require('ledger-ergo-js');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');
const { ADDRESS, ADDRESS2, CHANGE_ADDRESS, NETWORK } = require('./helpers/data');
const { createUnsignedTransaction, createUnsignedBox, createBoxCandidate, createDataInput, createChangeMap } = require('./helpers/transaction');

describe("Transaction Tests", function () {
    context("Transaction Commands", function () {
        it("can attest input", async function () {
            this.timeout(5000);
            const transaction = createUnsignedTransaction();
            const box = createUnsignedBox(ADDRESS, transaction.id(), 0);
            await this.screenFlows.attestInput.do(
                () => this.device.attestInput(box),
                attestedBox => {
                    expect(attestedBox).to.exist;
                }
            );
        });

        it("can sign tx", async function () {
            this.timeout(10000);
            const transaction1 = createUnsignedTransaction();
            const input1 = createUnsignedBox(ADDRESS, transaction1.id(), 0);
            const dataInput1 = createDataInput(ADDRESS, transaction1.id(), 0);
            const output1 = createBoxCandidate("500000", ADDRESS2);
            const changeMap = createChangeMap(CHANGE_ADDRESS, common.getAddressPath(0, 0, true), NETWORK);
            const unsignedTransaction = {
                inputs: [input1],
                dataInputs: [dataInput1],
                outputs: [output1],
                distinctTokenIds: [],
                changeMap,
            };
            const network = Network.Testnet;
            await this.screenFlows.signTx.do(
                () => this.device.signTx(unsignedTransaction, network),
                signedTransaction => {
                    expect(signedTransaction).to.exist;
                }
            );
        });
    })
});
