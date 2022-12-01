const { sleep } = require('./common');

function suppressPomiseError(promise) {
    return promise.catch((reason) => ({ __rejected: true, reason }));
}

function restorePromiseError(promise) {
    return promise.then((val) => {
        if (val.__rejected) {
            throw val.reason;
        }
        return val;
    });
}

class AuthTokenFlows {
    constructor(name, before, count = [1, 1]) {
        this.name = name;
        this.before = before;
        this.count = count;
    }

    do(action, success, failure) {
        const count = this.count;
        const before = this.before;

        success = success ?? (function (result) {
            throw new Error(`Success called: ${result}`);
        });
        failure = failure ?? (function (error) {
            throw new Error(`Failure called: ${error}`);
        });

        const run = auth => {
            it(`${this.name}${auth ? ' (with auth token)' : ''}`, async function () {
                this.timeout(30_000);
                const params = before();
                Object.assign(params, { test: this, auth });
                this.device.useAuthToken(auth);
                const promise = suppressPomiseError(action.call(params));
                const flows = [];
                for (let i = 0; i < count[auth]; i++) {
                    await sleep(1500); // Allow action to show UI
                    let flow = await this.screens.readFlow();
                    flows.push(flow);
                    await this.screens.clickOn('Approve');
                }
                Object.assign(params, { flows });
                let result;
                try {
                    result = await restorePromiseError(promise);
                } catch (error) {
                    failure.call(params, error);
                    return;
                }
                success.call(params, result);
            });
        }

        run(0);
        run(1);
    }
}

exports.AuthTokenFlows = AuthTokenFlows;
