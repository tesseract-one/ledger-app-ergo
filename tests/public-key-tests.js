const { expect } = require('chai')
    .use(require('chai-bytes'));
const { toHex, getApplication, removeMasterNode } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { AuthTokenFlows } = require('./helpers/flow');

describe("Public Key Tests", function () {
    context("Public Key Commands", function () {
        new AuthTokenFlows("can get extended public key", () => { return { account: TEST_DATA.account }; }).do(
            function () {
                return this.test.device.getExtendedPublicKey(this.account.path.toString())
            },
            function (extendedPublicKey) {
                const getExtendedPublicKeyFlow = [
                    { header: null, body: 'Ext PubKey Export' },
                    { header: 'Path', body: removeMasterNode(this.account.path.toString()) },
                    { header: null, body: 'Approve' },
                    { header: null, body: 'Reject' }
                ];
                if (this.auth) {
                    getExtendedPublicKeyFlow.splice(2, 0, { header: 'Application', body: getApplication(this.test.device) });
                }
                expect(this.flows[0]).to.be.deep.equal(getExtendedPublicKeyFlow);
                expect(extendedPublicKey).to.be.deep.equal({
                    publicKey: toHex(this.account.publicKey.pub_key_bytes()),
                    chainCode: toHex(this.account.publicKey.chain_code()),
                });
            }
        );
    });
});
