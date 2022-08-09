const chai = require('chai');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');

describe("Public Key Tests", function () {
    context("Public Key Commands", function () {
        it("can get extended public key", async function () {
            this.timeout(5000);
            const path = common.getAccountPath(0);
            await this.screenFlows.getExtendedPublicKey.do(
                () => this.device.getExtendedPublicKey(path),
                extendedPublicKey => {
                    expect(extendedPublicKey).to.be.deep.equal({
                        publicKey: '03c24e55008b523ccaf03b6c757f88c4881ef3331a255b76d2e078016c69c3dfd4',
                        chainCode: '8eb29c7897d57aee371bf254be6516e6963e2d9b379d0d626c17a39d1a3bf553',
                    });
                }
            );
        });
    })
});
