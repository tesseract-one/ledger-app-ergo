const makefile = require('./makefile');

const MAIN_FLOW = [
    { header: makefile.appName, body: "is ready" },
    { header: null, body: "About" },
    { header: null, body: "Quit" }
];

const ABOUT_FLOW = [
    { header: makefile.appName + " App", body: "(c) 2024 Ergo" },
    { header: "Version", body: makefile.version },
    { header: null, body: "Back" }
];

const SCREEN_LAST_LINE = {
    nanos: 17,
    nanox: 42,
    nanosp: 42
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
        this._model = model;
        this._currentScreen = {};

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
            // THIS IS THE UGLY HACK FOR BUGGY SPECULOS
            // Sometimes it doesn't send full MAIN screen, and sends only header (on quick operations)
            // We will set header 'null' for event
            if (screen.header === MAIN_FLOW[0].header && screen.body !== MAIN_FLOW[0].body) {
                screen.header = null;
                if (this._currentScreen.resolve) {
                    console.log("Ugly hack applied!");
                    this._currentScreen.resolve(MAIN_FLOW[0]);
                    this._currentScreen.resolve = null;
                    this._currentScreen.reject = null;
                } else {
                    console.log("ERROR! Problems ugly hack doesn't have Promise.");
                }
                if (!screen.body) {
                    console.log("ERROR! Ugly hack has empty screen body.");
                    return;
                }
            }
            // END OF THE UGLY HACK
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
            if (evt.y >= SCREEN_LAST_LINE[model]) {
                screenFinished();
            } else {
                timer = setTimeout(screenFinished, 200);
            }
        });
        this._automation.events.on("error", (err) => {
            this._currentScreen.reject(err);
        });
        this.removeCurrentScreen();
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

    async isReadyMainScreen() {
        const screen = await this.currentScreen();
        return mainMenuScreenIndex(screen) === 0;
    }

    removeCurrentScreen() {
        this._currentScreen = resolver();
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
        this.removeCurrentScreen();
        await this._automation.pressButton(type);
        return await this.currentScreen();
    }
}

exports.MAIN_FLOW = MAIN_FLOW;
exports.ABOUT_FLOW = ABOUT_FLOW;
exports.ScreenReader = ScreenReader;