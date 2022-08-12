class AuthTokenFlows {
    constructor(device, screens, count = [1, 1]) {
        this.device = device;
        this.screens = screens;
        this.count = count;
    }

    async do(action, success, failure) {
        const run = async auth => {
            this.device.useAuthToken(auth);
            const promise = action();
            let flows = [];
            for (let i = 0; i < this.count[auth]; i++) {
                flows.push(await this.screens.readFlow());
                await this.screens.click(0);
            }
            try {
                success(await promise, auth, flows);
            } catch (error) {
                failure(error, auth, flows);
            }
        }
        await run(0);
        await run(1);
    }
}

exports.AuthTokenFlows = AuthTokenFlows;
