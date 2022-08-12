const { assert, expect } = require('chai')
    .use(require('chai-bytes'));
const { toHex } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');

describe("Public Key Tests", function () {
    context("Public Key Commands", function () {
        it("can get extended public key", async function () {
            this.timeout(5000);
            const account = TEST_DATA.account;
            await this.screenFlows.getExtendedPublicKey.do(
                () => this.device.getExtendedPublicKey(account.path.toString()),
                extendedPublicKey => {
                    expect(extendedPublicKey).to.be.deep.equal({
                        publicKey: toHex(account.publicKey.pub_key_bytes()),
                        chainCode: toHex(account.publicKey.chain_code()),
                    });
                },
                error => assert.fail(error)
            );
        });
    })
});
