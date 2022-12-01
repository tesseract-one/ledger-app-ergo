const { expect } = require('chai')
    .use(require('chai-bytes'));
const { toHex, getApplication, removeMasterNode } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { mergePagedScreens } = require("./helper/screen");
const { AuthTokenFlows } = require('./helpers/flow');

describe("Address Tests", function () {
    context("Address Commands", function () {
        new AuthTokenFlows("can derive address", () => { return { address: TEST_DATA.address0 }; }).do(
            function () {
                return this.test.device.deriveAddress(this.address.path.toString());
            },
            function (derivedAddress) {
                const deriveAddressFlow = [
                    { header: null, body: 'Confirm Send Address' },
                    { header: 'Path', body: removeMasterNode(this.address.path.toString()) }
                ];
                if (this.auth) {
                    deriveAddressFlow.push({ header: 'Application', body: getApplication(this.test.device) });
                }
                expect(this.flows[0]).to.be.deep.equal(deriveAddressFlow);
                expect(derivedAddress).to.be.deep.equal({
                    addressHex: toHex(this.address.toBytes())
                });
            }
        );

        new AuthTokenFlows("can show address", () => { return { address: TEST_DATA.address0 }; }).do(
            function () {
                return this.test.device.showAddress(this.address.path.toString());
            },
            function (show) {
                const addressFlow = [
                    { header: null, body: 'Confirm Address' },
                    { header: 'Path', body: removeMasterNode(this.address.path.toString()) },
                    { header: 'Address', body: this.address.toBase58() },
                ];
                if (this.auth) {
                    addressFlow.push({ header: 'Application', body: getApplication(this.test.device) });
                }
                expect(mergePagedScreens(this.flows[0])).to.be.deep.equal(addressFlow);
                expect(show).to.be.true;
            }
        );
    });
});
