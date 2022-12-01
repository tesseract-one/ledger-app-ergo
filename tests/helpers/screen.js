const { sleep } = require('./common');
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
]

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
    constructor(automation) {
        this._automation = automation;
        this._currentScreen = {};

        let lines = [];
        let timer = undefined;
        const screenFinished = () => {
            const screen = { header: null, body: null };
            timer = undefined;
            if (lines.length === 1) {
                screen.body = lines.pop();
            } else {
                screen.header = lines.shift();
                screen.body = lines.reduce((acc, val) => acc + val, '');
                lines = [];
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
            lines.push(evt.text);
            timer = setTimeout(screenFinished, 100);
        });
        this._automation.events.on("error", (err) => {
            this._currentScreen.reject(err);
        });
        this._currentScreen = resolver();
    }

    async ensureMainMenu() {
        await this.goNext();
        let screen = await this.currentScreen();
        while (mainMenuScreenIndex(screen) >= 0 && mainMenuScreenIndex(screen) != 2) {
            await this.goNext();
            screen = await this.currentScreen();
        }
        if (mainMenuScreenIndex(screen) < 0) {
            return false;
        }
        while (mainMenuScreenIndex(screen) > 0) {
            await this.goPrevious();
            screen = await this.currentScreen();
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
        this._currentScreen = resolver();
        return this._automation.pressButton('right');
    }

    goPrevious() {
        this._currentScreen = resolver();
        return this._automation.pressButton('left');
    }

    async readFlow() {
        await sleep();
        let screens = [await this.currentScreen()];
        let screen = screens[0];
        do {
            await this.goNext();
            screen = await this.currentScreen();
            screens.push(screen);
        } while (screen.header !== screens[0].header && screen.body !== screens[0].body);
        screens.pop();
        return screens;
    }

    async click(index) {
        await sleep();
        for (let i = 0; i < index; i++) {
            await this.goNext();
            await this.currentScreen();
        }
        await this._automation.pressButton('both');
    }

    async clickOn(body) {
        await sleep();
        let firstScreen = await this.currentScreen();
        let currentScreen;
        do {
            await this.goNext();
            currentScreen = await this.currentScreen();
            if (currentScreen.body == body) {
                await this._automation.pressButton('both');
                break;
            }
        } while (currentScreen.header !== firstScreen.header
            && currentScreen.body !== firstScreen.body);
    }
}

exports.MAIN_FLOW = MAIN_FLOW;
exports.ABOUT_FLOW = ABOUT_FLOW;
exports.ScreenReader = ScreenReader;