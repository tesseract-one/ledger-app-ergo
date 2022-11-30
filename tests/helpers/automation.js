const http = require('http');
const EventEmitter = require('events');

class SpeculosAutomation {
    constructor(host, port) {
        this.host = host || "127.0.0.1";
        this.port = port;
        this.events = new EventEmitter();
        this.eventsReq = null;
    }

    connect() {
        return new Promise((resolve, reject) => {
            const req = http.request(
                { port: this.port, host: this.host, path: "/events?stream=true" },
                (res) => {
                    resolve();
                    res.on('error', err => events.emit('error', err));
                    res.on('data', data => {
                        const split = data.toString("ascii").split("\n");
                        split
                            .filter(ascii => !!ascii)
                            .map(str => str.slice(str.indexOf("{")))
                            .forEach((ascii) => {
                                const json = JSON.parse(ascii);
                                console.log("EVENT", json);
                                if (json.text) {
                                    this.events.emit('text', json);
                                }
                                this.events.emit('any', json);
                            });
                    });
                }
            );
            req.on('error', err => reject(err));
            req.setSocketKeepAlive(true);
            req.setHeader("Accept", "text/event-stream");
            req.end();
            this.eventsReq = req;
        });
    }

    pressButton(button) {
        return new Promise((resolve, reject) => {
            const message = '{"action":"press-and-release"}';
            const req = http.request(
                {
                    host: this.host,
                    port: this.port,
                    method: "POST",
                    path: "/button/" + button,
                },
                (res) => {
                    res.on('error', (err) => reject(err));
                    res.on('data', () => { });
                    res.on('end', () => resolve());
                });
            req.on('error', (err) => reject(err));
            req.setHeader("Accept", "application/json");
            req.setHeader("Content-Type", "application/json");
            req.setHeader("Content-Length", message.length);
            req.write(message);
            req.end();
        });
    }

    close() {
        this.eventsReq.destroy();
    }
}

exports.SpeculosAutomation = SpeculosAutomation;