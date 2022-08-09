const chai = require('chai');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');

describe("Public Key Tests", function () {
    context("Public Key Commands", function () {
        it("can get extended public key", async function () {
            const path = common.getAccountPath(0);
            const getExtendedPublicKey = this.device.getExtendedPublicKey(path);
            await common.sleep();
            if (this.screens) {
                await this.screens.click(3);
                const extendedPublicKey = await getExtendedPublicKey;
                expect(extendedPublicKey).to.be.deep.equal({
                    publicKey: '03c24e55008b523ccaf03b6c757f88c4881ef3331a255b76d2e078016c69c3dfd4',
                    chainCode: '8eb29c7897d57aee371bf254be6516e6963e2d9b379d0d626c17a39d1a3bf553',
                });
            } else {
                console.log("Check screens on the device, please!");
            }
        });
    })
});
