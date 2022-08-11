const chai = require('chai');
const { toHex } = require('./helpers/common');
const { expect } = chai.use(require('chai-bytes'));
const { TEST_DATA } = require('./helpers/data');

describe("Address Tests", function () {
    context("Address Commands", function () {
        it("can derive address", async function () {
            this.timeout(5000);
            await this.screenFlows.deriveAddress.do(
                () => this.device.deriveAddress(TEST_DATA.address0.path.toString()),
                address => {
                    expect(address).to.be.deep.equal({
                        addressHex: toHex(TEST_DATA.address0.toBytes())
                    });
                }
            );
        });

        it("can show address", async function () {
            this.timeout(5000);
            await this.screenFlows.showAddress.do(
                () => this.device.showAddress(TEST_DATA.address0.path.toString()),
                show => {
                    expect(show).to.be.true;
                }
            );
        });
    });
});
