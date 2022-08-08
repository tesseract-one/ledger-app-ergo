async function sleep(ms) {
    await new Promise(r => setTimeout(r, ms != null ? ms : 500));
}

function getAccountPath(account) {
    return `m/44'/429'/${account}'`
}

function getAddressPath(account, address) {
    return getAccountPath(account) + `/0/${address}`
}

exports.sleep = sleep;
exports.getAccountPath = getAccountPath;
exports.getAddressPath = getAddressPath;