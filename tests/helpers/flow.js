const { mergePagedScreens } = require('./screen');

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
            throw new Error(`Success called: ${JSON.stringify(result)}`);
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
                this.screens.removeCurrentScreen(); // Wait for new screen in the readFlow
                const promise = suppressPomiseError(action.call(params));
                const flows = [];
                for (let i = 0; i < count[auth]; i++) {
                    let flow = await this.screens.readFlow();
                    flows.push(mergePagedScreens(flow));
                    await this.screens.clickOn('Approve');
                    if (i != count[auth] - 1 && await this.screens.isReadyMainScreen()) { // we have more flows
                        this.screens.removeCurrentScreen(); // Wait for new screen in the readFlow
                    }
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
