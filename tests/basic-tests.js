const chai = require('chai');
const { expect } = chai.use(require('chai-bytes'));
const makefile = require('./helpers/makefile');
const screen = require('./helpers/screen');
const { sleep } = require("./helpers/common");

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
                this.timeout(10000);
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
