async function sleep(ms) {
    await new Promise(r => setTimeout(r, ms != null ? ms : 500));
}

function getAccountPath(account) {
    return `m/44'/429'/${account}'`
}

function getAddressPath(account, address, change) {
    return getAccountPath(account) + `/${change ? '1' : '0'}/${address}`
}

function toArray(object) {
    let array = [];
    for (let i = 0; i < object.len(); i++) {
        array.push(object.get(i));
    }
    return array;
}

exports.sleep = sleep;
exports.getAccountPath = getAccountPath;
exports.getAddressPath = getAddressPath;
exports.toArray = toArray;