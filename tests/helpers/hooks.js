const SpeculosTransport = require('@ledgerhq/hw-transport-node-speculos').default;
const HidTransport = require('@ledgerhq/hw-transport-node-hid').default;
const chai = require('chai');
const { expect } = chai.use(require('chai-bytes'));
const Device = require('./device').Device;

const APDU_PORT = 9999;
const BUTTON_PORT = 42000;
const AUTOMATION_PORT = 43000;

exports.mochaHooks = {
    beforeAll: async function () {
        if (process.env.LEDGER_LIVE_HARDWARE) {
            this.transport = await HidTransport.create();
            this.transport.button = console.log;
        } else {
            this.transport = await SpeculosTransport.open({
                apduPort: APDU_PORT,
                buttonPort: BUTTON_PORT,
                automationPort: AUTOMATION_PORT,
            });
        }
        this.device = new Device(this.transport);
    },
    afterAll: async function () {
        this.device = undefined;
        this.transport.close();
    }
};

global.expect = expect;