const SpeculosTransport = require('@ledgerhq/hw-transport-node-speculos').default;
const HidTransport = require('@ledgerhq/hw-transport-node-hid').default;
const { ErgoLedgerApp } = require('ledger-ergo-js');
const SpeculosAutomation = require('./automation').SpeculosAutomation;
const ScreenReader = require('./screen').ScreenReader;

const APDU_PORT = 9999;
const API_PORT = 40000;

exports.mochaHooks = {
    beforeAll: async function () {
        if (process.env.LEDGER_LIVE_HARDWARE) {
            this.transport = await HidTransport.create();
        } else {
            this.transport = await SpeculosTransport.open({
                apduPort: APDU_PORT,
            });
            this.automation = new SpeculosAutomation(null, API_PORT);
            await this.automation.connect();
            this.screens = new ScreenReader(this.automation);
        }
        this.device = new ErgoLedgerApp(this.transport);
    },
    afterAll: async function () {
        this.device = undefined;
        this.transport.close();
        if (this.automation) {
            this.automation.close();
        }
    }
};
