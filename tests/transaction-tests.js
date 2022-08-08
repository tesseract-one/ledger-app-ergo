const chai = require('chai');
const { Network } = require('ledger-ergo-js');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');
const { createUnsignedTransaction, toUnsignedBox, createErgoBox } = require('./helpers/transaction');

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
            const input1 = createUnsignedBox(transaction1.id(), 0);
            const output1 = {
                value: "500000",
                ergoTree: Buffer.from("0008cd033088e457b2ccd2d26e4c5df8bf3c0c332807ed7a9eb02a4b71affb576fb14210"),
                creationHeight: 0,
                tokens: [{
                    id: "0000000000000000000000000000000000000000000000000000000000000000",
                    amount: "1",
                }],
                registers: Buffer.from([]),
            };
            const changeMap = {
                address: "01033088e457b2ccd2d26e4c5df8bf3c0c332807ed7a9eb02a4b71affb576fb142106ba8dc41",
                path: common.getAddressPath(0, 1),
            };
            const unsignedTransaction = {
                inputs: [input1],
                dataInputs: ["0000000000000000000000000000000000000000000000000000000000000000"],
                outputs: [output1],
                distinctTokenIds: ["0000000000000000000000000000000000000000000000000000000000000000"],
                changeMap,
            }
            const network = Network.Testnet;
            const signTx = this.device.signTx(unsignedTransaction, network);
            await common.sleep();
            if (this.screens) {
                await this.screens.click(1);
                const signedTransaction = await signTx;
                expect(signedTransaction).to.exist;
            } else {
                console.log("Check screens on the device, please!");
            }
        });
    })
});
