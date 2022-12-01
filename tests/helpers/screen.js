const makefile = require('./makefile');

const MAIN_FLOW = [
    { header: makefile.appName, body: "is ready" },
    { header: null, body: "About" },
    { header: null, body: "Quit" }
];

const ABOUT_FLOW = [
    { header: makefile.appName + " App", body: "(c) 2021 Ergo" },
    { header: "Version", body: makefile.version },
    { header: null, body: "Back" }
];

const SCREEN_LINES = {
    nanos: 2,
    nanox: 4,
    nanosp: 4
};

function resolver() {
    let resolve, reject;
    let promise = new Promise((res, rej) => {
        resolve = res;
        reject = rej;
    });
    return { resolve, reject, promise };
}

function mainMenuScreenIndex(screen) {
    return MAIN_FLOW.findIndex((s) => s.body === screen.body && s.header == screen.header);
}

exports.findApproveScreenIndex = function (screens) {
    return screens.findIndex(s => s.header === null && s.body === "Approve");
}

exports.findRejectScreenIndex = function (screens) {
    return screens.findIndex(s => s.header === null && s.body === "Reject");
}

exports.mergePagedScreens = function (screens) {
    const regexp = /^(.*) \(([0-9]*)\/([0-9]*)\)$/;
    const fixed = [];
    for (let i = 0; i < screens.length; i++) {
        let m = screens[i].header && screens[i].header.match(regexp);
        if (m) {
            let header = m[1];
            let body = screens[i].body;
            while (m[2] !== m[3]) {
                i++;
                m = screens[i].header && screens[i].header.match(regexp);
                body = body + screens[i].body;
            }
            fixed.push({ header, body });
        } else {
            fixed.push(screens[i]);
        }
    }
    return fixed;
}

class ScreenReader {
    constructor(automation, model) {
        this._automation = automation;
        this._currentScreen = {};
        this._model = model;

        let events = [];
        let timer = undefined;
        const screenFinished = () => {
            const screen = { header: null, body: null };
            timer = undefined;
            if (events.length === 1) {
                screen.body = events.pop().text;
            } else {
                screen.header = events.shift().text;
                screen.body = events.reduce((acc, val) => acc + val.text, '');
                events = [];
            }
            if (this._currentScreen.resolve) {
                this._currentScreen.resolve(screen);
                this._currentScreen.resolve = null;
                this._currentScreen.reject = null;
            } else {
                this._currentScreen.promise = Promise.resolve(screen);
            }
        };
        this._automation.events.on("text", function (evt) {
            if (timer !== undefined) {
                clearTimeout(timer);
            }
            if (events.length === 0) {
                events.push(evt);
            } else {
                const last = events[events.length - 1];
                if (evt.y <= last.y) { screenFinished(); }
                events.push(evt);
            }
            if (events.length === SCREEN_LINES[model]) {
                screenFinished();
            } else {
                timer = setTimeout(screenFinished, 100);
            }
        });
        this._automation.events.on("error", (err) => {
            this._currentScreen.reject(err);
        });
        this._currentScreen = resolver();
    }

    async ensureMainMenu() {
        let screen = await this.goNext();
        while (mainMenuScreenIndex(screen) >= 0 && mainMenuScreenIndex(screen) != 2) {
            screen = await this.goNext();
        }
        if (mainMenuScreenIndex(screen) < 0) {
            return false;
        }
        while (mainMenuScreenIndex(screen) > 0) {
            screen = await this.goPrevious();
        }
        if (mainMenuScreenIndex(screen) < 0) {
            return false;
        }
        return true;
    }

    currentScreen() {
        return this._currentScreen.promise;
    }

    goNext() {
        return this.__pressAndWait('right');
    }

    goPrevious() {
        return this.__pressAndWait('left');
    }

    async readFlow() {
        let screens = [await this.currentScreen()];
        let screen = null;
        do {
            screen = await this.goNext();
            screens.push(screen);
        } while (screen.header !== screens[0].header || screen.body !== screens[0].body);
        screens.pop();
        return screens;
    }

    async click(index) {
        await this.currentScreen();
        for (let i = 0; i < index; i++) {
            await this.goNext();
        }
        await this.__pressAndWait('both');
    }

    async clickOn(body) {
        let firstScreen = await this.currentScreen();
        let currentScreen;
        do {
            currentScreen = await this.goNext();
            if (currentScreen.body === body) {
                await this.__pressAndWait('both');
                break;
            }
        } while (currentScreen.header !== firstScreen.header
            || currentScreen.body !== firstScreen.body);
    }

    async __pressAndWait(type) {
        this._currentScreen = resolver();
        await this._automation.pressButton(type);
        return await this.currentScreen();
    }
}

exports.MAIN_FLOW = MAIN_FLOW;
exports.ABOUT_FLOW = ABOUT_FLOW;
exports.ScreenReader = ScreenReader;