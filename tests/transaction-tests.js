const chai = require('chai');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');
const { createUnsignedTransaction } = require('./helpers/transaction');

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
    })
});
