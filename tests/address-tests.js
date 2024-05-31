const { expect } = require('chai')
    .use(require('chai-bytes'));
const { toHex, getApplication, removeMasterNode } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { mergePagedScreens } = require("./helpers/screen");
const { authTokenFlows } = require('./helpers/flow');

describe("Address Tests", function () {
    context("Address Commands", function () {
        authTokenFlows("can derive address")
            .init(async ({test, auth}) => {
                const address = TEST_DATA.address0;
                const flow = [{ header: null, body: 'Confirm Send Address' },
                               { header: 'Path', body: removeMasterNode(address.path.toString()) }];
                if (auth) {
                    flow.push({ header: 'Application', body: getApplication(test.device) })
                }
                flow.push({ header: null, body: 'Approve' }, { header: null, body: 'Reject' });
                return { address, flow, flowsCount: 1 };
            })
            .shouldSucceed(({flow, flows, address}, derived) => {
                expect(flows[0]).to.be.deep.equal(flow);
                expect(derived).to.be.deep.equal({
                    addressHex: toHex(address.toBytes())
                });
            })
            .run(({test, address}) => test.device.deriveAddress(address.path.toString()));

        authTokenFlows("can show address")
            .init(async ({test, auth}) => {
                const address = TEST_DATA.address0;
                const flow = [{ header: null, body: 'Confirm Address' },
                               { header: 'Path', body: removeMasterNode(address.path.toString()) },
                               { header: 'Address', body: address.toBase58() }];
                if (auth) {
                    flow.push({ header: 'Application', body: getApplication(test.device) })
                }
                flow.push({ header: null, body: 'Approve' }, { header: null, body: 'Reject' });
                return { address, flow, flowsCount: 1 };
            })
            .shouldSucceed(({flow, flows}, show) => {
                expect(mergePagedScreens(flows[0])).to.be.deep.equal(flow);
                expect(show).to.be.true;
            })
            .run(({test, address}) => test.device.showAddress(address.path.toString()));
    });
});
