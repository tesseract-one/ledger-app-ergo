class ScreenFlows {
    constructor(screens, device) {
        this.screens = screens;
        this.deriveAddress = new AuthFlow(
            [
                async () => {
                    await this.screens.click(2);
                },
                async () => {
                    await this.screens.click(3);
                },
            ], device
        );
        this.showAddress = new AuthFlow(
            [
                async () => {
                    await this.screens.click(5);
                },
                async () => {
                    await this.screens.click(6);
                },
            ], device
        );
        this.getExtendedPublicKey = new AuthFlow(
            [
                async () => {
                    await this.screens.click(2);
                },
                async () => {
                    await this.screens.click(3);
                },
            ], device
        );
        this.attestInput = new AuthFlow(
            [
                async () => {
                    await this.screens.click(1);
                },
                async () => {
                    await this.screens.click(2);
                },
            ], device
        );
        this.signTx = new AuthFlow(
            [
                async () => {
                    await this.screens.click(1);
                    await this.screens.click(2);
                    await this.screens.click(4);
                },
                async () => {
                    await this.screens.click(2);
                    await this.screens.click(4);
                },
            ], device
        );
    }
}

class AuthFlow {
    constructor(flows, device) {
        this.flows = flows;
        this.device = device;
    }

    async do(action, result) {
        const run = async auth => {
            this.device.useAuthToken(auth);
            const promise = action();
            await this.flows[auth]();
            result(await promise);
        }
        await run(1);
        await run(0);
    }
}

exports.ScreenFlows = ScreenFlows;
