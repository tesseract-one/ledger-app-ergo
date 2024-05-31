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
    constructor(name) {
        this._name = name;
        this._before = null;
        this.shouldSucceed(null);
        this.shouldFail(null);
    }

    init(before) {
        this._before = before;
        return this;
    }

    shouldSucceed(success) {
        this._success = success ?? (function (_, result) {
            throw new Error(`Success called: ${JSON.stringify(result)}`);
        });
        return this;
    }

    shouldFail(failure) {
        this._failure = failure ?? (function (_, error) {
            throw new Error(`Failure called: ${error}`);
        });
        return this;
    }

    run(action) {
        this.__run(false, action);
        this.__run(true, action);
    }

    __run(auth, action) {
        const before = this._before;
        const success = this._success;
        const failure = this._failure;

        it(`${this._name}${auth ? ' (with auth token)' : ''}`, async function() {
            this.timeout(30_000);
            this.device.useAuthToken(auth);
            const params = { test: this, auth };
            if (before) {
                Object.assign(params, await before(params));
            }
            const flowsCount = params.flowsCount;
            if (typeof flowsCount !== "number") {
                throw new Error("flowsCount should be defined and to be a number");
            }
            this.screens.removeCurrentScreen(); // Wait for new screen in the readFlow
            // Call action
            const promise = suppressPomiseError(action(params));
            // Read flows
            const flows = [];
            for (let i = 0; i < flowsCount; i++) {
                let flow = await this.screens.readFlow();
                flows.push(mergePagedScreens(flow));
                await this.screens.clickOn('Approve');
                if (i != flowsCount - 1 && await this.screens.isReadyMainScreen()) { // we have more flows
                    this.screens.removeCurrentScreen(); // Wait for new screen in the readFlow
                }
            }
            params.flows = flows;
            // Call success or error
            let result;
            try {
                result = await restorePromiseError(promise);
            } catch (error) {
                failure(params, error);
                return;
            }
            success(params, result);
        });
        return null;
    }
}

exports.authTokenFlows = function(name) {
    return new AuthTokenFlows(name);
};

exports.AuthTokenFlows = AuthTokenFlows;
