class AuthTokenFlows {
    constructor(name, before, count = [1, 1]) {
        this.name = name;
        this.before = before;
        this.count = count;
    }

    do(action, success, failure) {
        const count = this.count;
        const params = this.before();
        const run = auth => {
            it(`${this.name}${auth ? ' (with auth token)' : ''}`, async function () {
                this.timeout(10000);
                Object.assign(params, { test: this, auth });
                this.device.useAuthToken(auth);
                const promise = action.call(params);
                const flows = [];
                for (let i = 0; i < count[auth]; i++) {
                    flows.push(await this.screens.readFlow());
                    await this.screens.click(0);
                }
                Object.assign(params, { flows });
                try {
                    success.call(params, await promise);
                } catch (error) {
                    if (failure) {
                        failure.call(params, error);
                    } else {
                        throw error;
                    }
                }
            });
        }
        run(0);
        run(1);
    }
}

exports.AuthTokenFlows = AuthTokenFlows;
