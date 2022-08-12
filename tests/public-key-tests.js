const { assert, expect } = require('chai')
    .use(require('chai-bytes'));
const { toHex, getApplication, removeMasterNode } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { AuthTokenFlows } = require('./helpers/flow');

describe("Public Key Tests", function () {
    context("Public Key Commands", function () {
        it("can get extended public key", async function () {
            this.timeout(5000);
            const account = TEST_DATA.account;
            const getExtendedPublicKeyFlow = auth => {
                const flow = [
                    { header: null, body: 'Ext PubKey Export' },
                    { header: 'Path', body: removeMasterNode(account.path.toString()) }
                ];
                if (auth) {
                    flow.push({ header: 'Application', body: getApplication(this.device) });
                }
                return flow;
            };
            await new AuthTokenFlows(this.device, this.screens).do(
                () => this.device.getExtendedPublicKey(account.path.toString()),
                (extendedPublicKey, auth, [flow]) => {
                    expect(flow).to.be.deep.equal(getExtendedPublicKeyFlow(auth));
                    expect(extendedPublicKey).to.be.deep.equal({
                        publicKey: toHex(account.publicKey.pub_key_bytes()),
                        chainCode: toHex(account.publicKey.chain_code()),
                    });
                },
                error => assert.fail(error)
            );
        });
    });
});
