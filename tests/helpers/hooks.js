const SpeculosTransport = require('@ledgerhq/hw-transport-node-speculos').default;
const HidTransport = require('@ledgerhq/hw-transport-node-hid').default;
const { ErgoLedgerApp } = require('ledger-ergo-js');
const { ScreenFlows } = require('./flow');
const SpeculosAutomation = require('./automation').SpeculosAutomation;
const ScreenReader = require('./screen').ScreenReader;

const APDU_PORT = 9999;
const API_PORT = 40000;

exports.mochaHooks = {
    beforeAll: async function () {
        this.model = process.env.npm_config_model;
        if (this.model === "device") {
            this.transport = await HidTransport.create();
        } else {
            this.transport = await SpeculosTransport.open({
                apduPort: APDU_PORT,
            });
            this.automation = new SpeculosAutomation(null, API_PORT);
            await this.automation.connect();
            this.screens = new ScreenReader(this.automation);
            this.device = new ErgoLedgerApp(this.transport);
        }
    },
    beforeEach: async function () {
        if (this.screens) {
            this.timeout(2000);
            if (!await this.screens.ensureMainMenu()) {
                throw new Error("Emulator shows not a main flow!")
            }
        }
    },
    afterAll: async function () {
        this.device = undefined;
        this.transport.close();
        if (this.automation) {
            this.automation.close();
        }
    }
};
