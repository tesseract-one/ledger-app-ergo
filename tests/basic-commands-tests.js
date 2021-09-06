const Buffer = require('buffer').Buffer;

describe("Basic Commands Tests", function () {
    context("App Version", function () {
        it("can fetch version of the app", async function () {
            const version = await this.device.getAppVersion();
            expect(version).to.be.equal(Buffer.from([0x00, 0x00, 0x01, 0x01]));
        });
    });

    context("App Name", function () {
        it("can fetch name of the app", async function () {
            const name = await this.device.getAppName();
            expect(name).to.be.equal("Ergo");
        });
    });
});