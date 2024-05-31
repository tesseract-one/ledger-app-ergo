const { expect } = require('chai')
    .use(require('chai-bytes'));
const { toHex, getApplication, removeMasterNode } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { authTokenFlows } = require('./helpers/flow');

describe("Public Key Tests", function () {
    context("Public Key Commands", function () {
        authTokenFlows("can get extended public key")
            .init(async ({test, auth}) => {
                const account = TEST_DATA.account;
                const flow = [{ header: null, body: 'Ext PubKey Export' },
                              { header: 'Path', body: removeMasterNode(account.path.toString()) }];
                if (auth) {
                    flow.push({ header: 'Application', body: getApplication(test.device) });
                }
                flow.push({ header: null, body: 'Approve' }, { header: null, body: 'Reject' });
                return { account, flow, flowsCount: 1 };
            })
            .shouldSucceed(({flow, flows, account}, extPubKey) => {
                expect(flows[0]).to.be.deep.equal(flow);
                expect(extPubKey).to.be.deep.equal({
                    publicKey: toHex(account.publicKey.pub_key_bytes()),
                    chainCode: toHex(account.publicKey.chain_code()),
                });
            })
            .run(({test, account}) => test.device.getExtendedPublicKey(account.path.toString()));
    });
});
