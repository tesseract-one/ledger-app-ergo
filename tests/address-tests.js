const chai = require('chai');
const { expect } = chai.use(require('chai-bytes'));
const common = require('./helpers/common');

describe("Address Tests", function () {
    context("Address Commands", function () {
        it("can derive address", async function () {
            const path = common.getAddressPath(0, 0);
            const deriveAddress = this.device.deriveAddress(path);
            await common.sleep();
            if (this.screens) {
                await this.screens.click(2);
                const address = await deriveAddress;
                expect(address).to.be.deep.equal({
                    addressHex: '01033088e457b2ccd2d26e4c5df8bf3c0c332807ed7a9eb02a4b71affb576fb142106ba8dc41'
                });
            } else {
                console.log("Check screens on the device, please!");
            }
        });

        it("can show address", async function () {
            const path = common.getAddressPath(0, 0);
            const showAddress = this.device.showAddress(path);
            await common.sleep();
            if (this.screens) {
                await this.screens.click(5);
                const show = await showAddress;
                expect(show).to.be.true;
            } else {
                console.log("Check screens on the device, please!");
            }
        });
    });
});
