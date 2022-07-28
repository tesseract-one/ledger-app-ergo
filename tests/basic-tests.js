const chai = require('chai');
const { expect } = chai.use(require('chai-bytes'));
const makefile = require('./helpers/makefile');
const screen = require('./helpers/screen');

async function sleep(ms) {
    await new Promise(r => setTimeout(r, ms));
}

function getAccountPath(account) {
    return `m/44'/429'/${account}'`
}

function getAddressPath(account, address) {
    return getAccountPath(account) + `/0/${address}`
}

describe("Basic Tests", function () {
    context("Basic Commands", function () {
        it("can fetch version of the app", async function () {
            const version = await this.device.getAppVersion();
            expect(version).to.be.deep.equal({
                major: makefile.versionMajor,
                minor: makefile.versionMinor,
                patch: makefile.versionPatch,
                flags: { isDebug: true }
            });
        });

        it("can fetch name of the app", async function () {
            const name = (await this.device.getAppName()).name;
            expect(name).to.be.equal(makefile.appName);
        });

        it("can get extended public key", async function () {
            const path = getAccountPath(0);
            const getExtendedPublicKey = this.device.getExtendedPublicKey(path);
            await sleep(500);
            if (this.screens) {
                await this.screens.click(2);
                const extendedPublicKey = await getExtendedPublicKey;
                expect(extendedPublicKey).to.have.property('publicKey').that.exist;
                expect(extendedPublicKey).to.have.property('chainCode').that.exist;
            } else {
                console.log("Check screens on the device, please!");
            }
        });

        it("can derive address", async function () {
            const path = getAddressPath(0, 0);
            const deriveAddress = this.device.deriveAddress(path);
            await sleep(500);
            if (this.screens) {
                await this.screens.click(2);
                const address = await deriveAddress;
                expect(address).to.have.property('addressHex').that.exist;
            } else {
                console.log("Check screens on the device, please!");
            }
        });

        it("can show address", async function () {
            const path = getAddressPath(0, 0);
            const showAddress = this.device.showAddress(path);
            await sleep(500);
            if (this.screens) {
                await this.screens.click(5);
                const show = await showAddress;
                expect(show).to.be.true;
            } else {
                console.log("Check screens on the device, please!");
            }
        });
    });

    context("Basic Flows", function () {
        it("main flow is working", async function () {
            if (this.screens) {
                this.timeout(5000);
                const main = await this.screens.ensureMainMenu();
                expect(main).to.be.equal(true);
            } else {
                console.log("Check screens on the device, please!");
                console.log("Screens", screen.MAIN_FLOW);
            }
        });

        it("about flow is working", async function () {
            if (this.screens) {
                this.timeout(5000);
                const main = await this.screens.ensureMainMenu();
                expect(main).to.be.equal(true);
                await this.screens.click(1);
                const aboutMenu = await this.screens.readFlow();
                await this.screens.click(2);
                expect(aboutMenu).to.be.deep.equal(screen.ABOUT_FLOW);
            } else {
                console.log("Check screens on the device, please!");
                console.log("Screens", screen.ABOUT_FLOW);
            }
        });
    });
});