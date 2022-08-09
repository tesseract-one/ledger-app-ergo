const chai = require('chai');
const { Network } = require('ledger-ergo-js');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');
const { ADDRESS, ADDRESS2, CHANGE_ADDRESS, NETWORK } = require('./helpers/data');
const { createUnsignedTransaction, createUnsignedBox, createBoxCandidate, createDataInput, createChangeMap } = require('./helpers/transaction');

describe("Transaction Tests", function () {
    context("Transaction Commands", function () {
        it("can attest input", async function () {
            const transaction = createUnsignedTransaction();
            const box = {
                txId: transaction.id().to_str(),
                index: 0,
                value: "1000000",
                ergoTree: Buffer.from("0008cd033088e457b2ccd2d26e4c5df8bf3c0c332807ed7a9eb02a4b71affb576fb14210"),
                creationHeight: 0,
                tokens: [],
                additionalRegisters: Buffer.from([]),
                extension: Buffer.from([]),
                signPath: "",
            };
            const attestInput = this.device.attestInput(box);
            await common.sleep();
            if (this.screens) {
                await this.screens.click(1);
                const attestedBox = await attestInput;
                expect(attestedBox).to.exist;
            } else {
                console.log("Check screens on the device, please!");
            }
        });

        it("can sign tx", async function () {
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
            const signTx = this.device.signTx(unsignedTransaction, network);
            await common.sleep();
            if (this.screens) {
                this.timeout(5000);
                await this.screens.click(1);
                await common.sleep();
                await this.screens.click(4);
                const signedTransaction = await signTx;
                expect(signedTransaction).to.exist;
            } else {
                console.log("Check screens on the device, please!");
            }
        });
    })
});
