const chai = require('chai');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');

describe("Address Tests", function () {
    context("Address Commands", function () {
        it("can derive address", async function () {
            this.timeout(5000);
            const path = common.getAddressPath(0, 0);
            await this.screenFlows.deriveAddress.do(
                () => this.device.deriveAddress(path),
                address => {
                    expect(address).to.be.deep.equal({
                        addressHex: '01033088e457b2ccd2d26e4c5df8bf3c0c332807ed7a9eb02a4b71affb576fb142106ba8dc41'
                    });
                }
            );
        });

        it("can show address", async function () {
            this.timeout(5000);
            const path = common.getAddressPath(0, 0);
            await this.screenFlows.showAddress.do(
                () => this.device.showAddress(path),
                show => {
                    expect(show).to.be.true;
                }
            );
        });
    });
});
