const path = require('path');
const fs = require('fs');

const makefile = fs.readFileSync(path.join(__dirname, "..", "..", "Makefile"), { encoding: "utf-8" });

exports.versionMajor = parseInt(makefile.match(/^APPVERSION_M\s+=\s+([0-9]+)$/m)[1], 10);
exports.versionMinor = parseInt(makefile.match(/^APPVERSION_N\s+=\s+([0-9]+)$/m)[1], 10);
exports.versionPatch = parseInt(makefile.match(/^APPVERSION_P\s+=\s+([0-9]+)$/m)[1], 10);
exports.version = `${exports.versionMajor}.${exports.versionMinor}.${exports.versionPatch}`;

exports.appName = makefile.match(/^APPNAME\s+=\s+\"(.+?)\"$/m)[1]