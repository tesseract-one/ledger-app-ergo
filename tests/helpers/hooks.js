const SpeculosTransport = require('@ledgerhq/hw-transport-node-speculos-http').default;
const HidTransport = require('@ledgerhq/hw-transport-node-hid').default;
const { ErgoLedgerApp } = require('ledger-ergo-js');
const SpeculosAutomation = require('./automation').SpeculosAutomation;
const ScreenReader = require('./screen').ScreenReader;

const API_PORT = 5000;

exports.mochaHooks = {
    beforeAll: async function () {
        this.model = process.env.npm_config_model;
        const port = process.env.npm_config_port ?? API_PORT;
        if (!this.model) {
            throw new Error("No model. Provide model with --model=<model> parameter");
        }
        if (["nanos", "nanox", "nanosp", "hid"].indexOf(this.model) < 0) {
            throw new Error("Unknown model: "+this.model+", supports: nanos, nanox, nanosp, hid");
        }
        if (this.model === "hid") {
            this.transport = await HidTransport.create();
        } else {
            this.transport = await SpeculosTransport.open({
                baseURL: "http://127.0.0.1:" + port
            });
            this.automation = new SpeculosAutomation(this.transport);
            this.screens = new ScreenReader(this.automation, this.model);
            this.device = new ErgoLedgerApp(this.transport);
        }
    },
    afterAll: async function () {
        this.device = undefined;
        this.screens = undefined;
        this.transport.close();
        if (this.automation) {
            this.automation.close();
        }
    }
};
