const EventEmitter = require('events');

class SpeculosAutomation {
    constructor(transport) {
        this.transport = transport
        this.events = new EventEmitter();;
        this.transport.eventStream.on('data', (data) => this._data(data));
        this.transport.eventStream.on('error', (err) => this.events.emit('error', err));
    }

    _data(data) {
        const split = data.toString("ascii").split("\n");
        split
            .filter(ascii => !!ascii)
            .map(str => str.slice(str.indexOf("{")))
            .forEach((ascii) => {
                const json = JSON.parse(ascii);
                if (json.text) {
                    this.events.emit('text', json);
                }
                this.events.emit('any', json);
            });
    }

    pressButton(button) {
        return this.transport.button(button);
    }

    close() {
        this.events = null;
        this.transport = null;
    }
}

exports.SpeculosAutomation = SpeculosAutomation;