const { assert, expect } = require('chai')
    .use(require('chai-bytes'));
const { toHex, getApplication, removeMasterNode } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { AuthTokenFlows } = require('./helpers/flow');

describe("Address Tests", function () {
    context("Address Commands", function () {
        it("can derive address", async function () {
            this.timeout(5000);
            const address = TEST_DATA.address0;
            const deriveAddressFlow = auth => {
                const flow = [
                    { header: null, body: 'Confirm Send Address' },
                    { header: 'Path', body: removeMasterNode(address.path.toString()) }
                ];
                if (auth) {
                    flow.push({ header: 'Application', body: getApplication(this.device) });
                }
                return flow;
            };
            await new AuthTokenFlows(this.device, this.screens).do(
                () => this.device.deriveAddress(address.path.toString()),
                (derivedAddress, auth, [flow]) => {
                    expect(flow).to.be.deep.equal(deriveAddressFlow(auth));
                    expect(derivedAddress).to.be.deep.equal({
                        addressHex: toHex(address.toBytes())
                    });
                },
                error => assert.fail(error)
            );
        });

        it("can show address", async function () {
            this.timeout(5000);
            const address = TEST_DATA.address0;
            const showAddressFlow = auth => {
                const flow = [
                    { header: null, body: 'Confirm Address' },
                    { header: 'Path', body: removeMasterNode(address.path.toString()) },
                    { header: 'Address (1/3)', body: address.toBase58().substring(0, 19) },
                    { header: 'Address (2/3)', body: address.toBase58().substring(19, 36) },
                    { header: 'Address (3/3)', body: address.toBase58().substring(36) }
                ];
                if (auth) {
                    flow.push({ header: 'Application', body: getApplication(this.device) });
                }
                return flow;
            };
            await new AuthTokenFlows(this.device, this.screens).do(
                () => this.device.showAddress(address.path.toString()),
                (show, auth, [flow]) => {
                    expect(flow).to.be.deep.equal(showAddressFlow(auth));
                    expect(show).to.be.true;
                },
                error => assert.fail(error)
            );
        });
    });
});
