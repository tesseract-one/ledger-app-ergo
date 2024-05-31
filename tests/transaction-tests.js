const { expect } = require('chai').use(require('chai-bytes'));
const { Transaction, ErgoBox } = require('ergo-lib-wasm-nodejs');
const { toNetwork, getApplication, removeMasterNode, ellipsize } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { authTokenFlows } = require('./helpers/flow');
const { TxBuilder } = require('./helpers/transaction');

const txId = "0000000000000000000000000000000000000000000000000000000000000000";

function signTxFlows({ device }, auth, from, to, change, tokens_to = undefined, tokens_tx = undefined) {
    let i = 0;
    const flows = [];
    // attest input screen
    flows[i] = [{ header: null, body: 'Confirm Attest Input' }];
    if (auth) {
        flows[i].push({ header: 'Application', body: getApplication(device) });
    }
    flows[i++].push({ header: null, body: 'Approve' }, { header: null, body: 'Reject' });
    // accept tx screen
    flows[i] = [{ header: 'P2PK Signing', body: removeMasterNode(from.path.toString()) }];
    if (!auth) {
        flows[i].push({ header: 'Application', body: '0x00000000' });
    }
    flows[i++].push({ header: null, body: 'Approve' }, { header: null, body: 'Reject' });
    // output screen
    if (to) {
        flows[i] = [{ header: null, body: 'Confirm Output' },
                { header: 'Address', body: to.toBase58() },
                { header: 'Output Value', body: '0.100000000 ERG' }];
        if (tokens_to) { flows[i].push(...tokens_to); }
        flows[i++].push({ header: null, body: 'Approve' }, { header: null, body: 'Reject' });
    }
    // change screen
    if (change && (from.acc_index != change.acc_index || change.addr_index >= 19)) {
        flows[i++] = [{ header: null, body: 'Confirm Output' },
                      { header: 'Change', body: removeMasterNode(change.path.toString()) },
                      { header: null, body: 'Approve' },
                      { header: null, body: 'Reject' }];
    }
    if (to && change) {
        flows[i] = [{ header: null, body: 'Approve Signing' },
                { header: 'P2PK Path', body: removeMasterNode(from.path.toString()) },
                { header: 'Transaction Amount', body: '0.100000000 ERG' },
                { header: 'Transaction Fee', body: '0.001000000 ERG' }];
        if (tokens_tx) { flows[i].push(...tokens_tx); }
        flows[i++].push({ header: null, body: 'Approve' }, { header: null, body: 'Reject' });
    }
    return flows;
}

function verifySignatures(unsigned, signatures, ergoBox) {
    const signed = Transaction.from_unsigned_tx(unsigned, signatures);
    const verificationResult = signed.verify_p2pk_input(ergoBox);
    expect(verificationResult).to.be.true;
}

describe("Transaction Tests", function () {
    context("Transaction Commands", function () {
        authTokenFlows("can attest input")
            .init(async ({test, auth}) => {
                const unsignedBox = new TxBuilder()
                    .input(TEST_DATA.address0, txId, 0, '1000000000')
                    .inputs[0].box;
                const flow = [{ header: null, body: 'Confirm Attest Input' }];
                if (auth) {
                    flow.push({ header: 'Application', body: getApplication(test.device) });
                }
                flow.push({ header: null, body: 'Approve' }, { header: null, body: 'Reject' });
                return { unsignedBox, flow, flowsCount: 1 };
            })
            .shouldSucceed(({flow, flows, unsignedBox}, attestedBox) => {
                expect(flows[0]).to.be.deep.equal(flow);
                expect(attestedBox).to.have.property('box');
                expect(attestedBox.box).to.be.deep.equal(unsignedBox);
                expect(attestedBox).to.have.property('frames');
                expect(attestedBox.frames).to.have.length(1);
                const frame = attestedBox.frames[0];
                expect(frame.boxId).to.exist;
                expect(frame.framesCount).to.be.equal(1);
                expect(frame.frameIndex).to.be.equal(0);
                expect(frame.amount).to.be.equal('1000000000');
                expect(frame.tokens).to.be.empty;
                expect(frame.attestation).to.exist;
                expect(frame.buffer).to.exist;
            })
            .run(({test, unsignedBox}) => test.device.attestInput(unsignedBox));

        authTokenFlows("can sign tx")
            .init(async ({test, auth}) => {
                const from = TEST_DATA.address0;
                const to = TEST_DATA.address1;
                const change = TEST_DATA.changeAddress;
                const {appTx, ergoTx, uInputs} = new TxBuilder()
                    .input(from, txId, 0, '1000000000')
                    .dataInput(from.address, txId, 1)
                    .output(to.address, '100000000')
                    .fee('1000000')
                    .change(change)
                    .build();
                const expectedFlows = signTxFlows(test, auth, from, to, change);
                return { appTx, ergoTx, input: uInputs[0],
                         expectedFlows, flowsCount: expectedFlows.length,
                         network: TEST_DATA.network };
            })
            .shouldSucceed(({ergoTx, input, expectedFlows, flows}, signatures) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(signatures).to.have.length(1);
                verifySignatures(ergoTx, signatures, input);
            })
            .run(({test, appTx, network}) => test.device.signTx(appTx, toNetwork(network)));

        authTokenFlows("can sign tx change 22")
            .init(async ({test, auth}) => {
                const from = TEST_DATA.address0;
                const to = TEST_DATA.address1;
                const change = TEST_DATA.changeAddress22;
                const {appTx, ergoTx, uInputs} = new TxBuilder()
                    .input(from, txId, 0, '1000000000')
                    .dataInput(from.address, txId, 1)
                    .output(to.address, '100000000')
                    .fee('1000000')
                    .change(change)
                    .build();
                const expectedFlows = signTxFlows(test, auth, from, to, change);
                return { appTx, ergoTx, input: uInputs[0],
                         expectedFlows, flowsCount: expectedFlows.length };
            })
            .shouldSucceed(({ergoTx, input, expectedFlows, flows}, signatures) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(signatures).to.have.length(1);
                verifySignatures(ergoTx, signatures, input);
            })
            .run(({test, appTx}) => test.device.signTx(appTx, toNetwork(TEST_DATA.network)));

        authTokenFlows("can sign tx with additional registers")
            .init(({test, auth}) => {
                const from = TEST_DATA.address0;
                const to = TEST_DATA.address1;
                const change = TEST_DATA.changeAddress;
                const ergoBox = ErgoBox.from_json(`{
                    "boxId": "ef16f4a6db61a1c31aea55d3bf10e1fb6443cf08cff4a1cf2e3a4780e1312dba",
                    "value": 1000000000,
                    "ergoTree": "${from.address.to_ergo_tree().to_base16_bytes()}",
                    "assets": [],
                    "additionalRegisters": {
                    "R5": "0e050102030405",
                    "R4": "04f601"
                    },
                    "creationHeight": 0,
                    "transactionId": "0000000000000000000000000000000000000000000000000000000000000000",
                    "index": 0
                }`);
                const {appTx, ergoTx, uInputs} = new TxBuilder()
                    .boxInput(ergoBox, from.path)
                    .output(to.address, '100000000')
                    .fee('1000000')
                    .change(change)
                    .build();
                const expectedFlows = signTxFlows(test, auth, from, to, change);
                return { appTx, ergoTx, input: uInputs[0],
                         expectedFlows, flowsCount: expectedFlows.length };
            })
            .shouldSucceed(({ergoTx, input, expectedFlows, flows}, signatures) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(signatures).to.have.length(1);
                verifySignatures(ergoTx, signatures, input);
            })
            .run(({test, appTx}) => test.device.signTx(appTx, toNetwork(TEST_DATA.network)));

        authTokenFlows("can sign tx with zero data inputs")
            .init(async ({test, auth}) => {
                const from = TEST_DATA.address0;
                const to = TEST_DATA.address1;
                const change = TEST_DATA.changeAddress;
                const {appTx, ergoTx, uInputs} = new TxBuilder()
                    .input(from, txId, 0, '1000000000')
                    .output(to.address, '100000000')
                    .fee('1000000')
                    .change(change)
                    .build();
                const expectedFlows = signTxFlows(test, auth, from, to, change);
                return { appTx, ergoTx, input: uInputs[0],
                         expectedFlows, flowsCount: expectedFlows.length };
            })
            .shouldSucceed(({ergoTx, input, expectedFlows, flows}, signatures) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(signatures).to.have.length(1);
                verifySignatures(ergoTx, signatures, input);
            })
            .run(({test, appTx}) => test.device.signTx(appTx, toNetwork(TEST_DATA.network)));

        authTokenFlows("can not sign tx with zero inputs")
            .init(async () => {
                const tx = new TxBuilder()
                    .dataInput(TEST_DATA.address0.address, txId, 0)
                    .output(TEST_DATA.address1.address, '100000000')
                    .fee(null)
                    .buildAppTx();
                return { tx, flowsCount: 0 };
            })
            .shouldFail((_, error) => {
                expect(error).to.be.an('error');
                expect(error.name).to.be.equal('DeviceError');
                expect(error.message).to.be.equal('Bad input count');
            })
            .run(({test, tx}) => test.device.signTx(tx, toNetwork(TEST_DATA.network)));

        authTokenFlows("can not sign tx with zero outputs")
            .init(async ({test, auth}) => {
                const from = TEST_DATA.address0;
                const tx = new TxBuilder()
                    .input(from, txId, 0, '1000000000')
                    .dataInput(from.address, txId, 0)
                    .fee(null)
                    .buildAppTx();
                const expectedFlows = signTxFlows(test, auth, from, null, null);
                return { tx, expectedFlows, flowsCount: expectedFlows.length };
            })
            .shouldFail(({flows, expectedFlows}, error) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(error).to.be.an('error');
                expect(error.name).to.be.equal('DeviceError');
                expect(error.message).to.be.equal('Bad output count');
            })
            .run(({test, tx}) => test.device.signTx(tx, toNetwork(TEST_DATA.network)));

        authTokenFlows("can sign tx with tokens")
            .init(async ({test, auth}) => {
                const from = TEST_DATA.address0;
                const to = TEST_DATA.address1;
                const change = TEST_DATA.changeAddress;
                const tokenId = '1111111111111111111111111111111111111111111111111111111111111111';
                const tokens = [{id: tokenId, amount: '1000'}];
                const {appTx, ergoTx, uInputs} = new TxBuilder()
                    .input(from, txId, 0, '1000000000', tokens)
                    .dataInput(from.address, txId, 0)
                    .output(to.address, '100000000', tokens)
                    .fee('1000000')
                    .change(change)
                    .build();
                const tokensFlow = [
                    { header: 'Token [1]', body: ellipsize(test.model, tokenId) },
                    { header: 'Token [1] Value', body: '1000' }
                ];
                const expectedFlows = signTxFlows(test, auth, from, to, change, tokensFlow);
                return { appTx, ergoTx, input: uInputs[0],
                         expectedFlows, flowsCount: expectedFlows.length };
            })
            .shouldSucceed(({ergoTx, input, flows, expectedFlows}, signatures) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(signatures).to.have.length(1);
                verifySignatures(ergoTx, signatures, input);
            })
            .run(({test, appTx}) => test.device.signTx(appTx, toNetwork(TEST_DATA.network)));

        authTokenFlows("can sign tx with burned tokens")
            .init(async ({test, auth}) => {
                const from = TEST_DATA.address0;
                const to = TEST_DATA.address1;
                const change = TEST_DATA.changeAddress;
                const tokenId = '1111111111111111111111111111111111111111111111111111111111111111';
                const tokens = [{id: tokenId, amount: '1000'}];
                const {appTx, ergoTx, uInputs} = new TxBuilder()
                    .input(from, txId, 0, '1000000000', tokens)
                    .dataInput(from.address, txId, 0)
                    .output(to.address, '100000000')
                    .fee('1000000')
                    .change(change)
                    .burn(tokens)
                    .build();
                const tokensFlow = [
                    { header: 'Token [1]', body: ellipsize(test.model, tokenId) },
                    { header: 'Token [1] Value', body: 'Burning: 1000' }
                ];
                const expectedFlows = signTxFlows(test, auth, from, to, change, null, tokensFlow);
                return { appTx, ergoTx, input: uInputs[0],
                         expectedFlows, flowsCount: expectedFlows.length };;
            })
            .shouldSucceed(({flows, expectedFlows, ergoTx, input}, signatures) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(signatures).to.have.length(1);
                verifySignatures(ergoTx, signatures, input);
            })
            .run(({test, appTx}) => test.device.signTx(appTx, toNetwork(TEST_DATA.network)));

        authTokenFlows("can sign tx with minted tokens")
            .init(async ({test, auth}) => {
                const from = TEST_DATA.address0;
                const to = TEST_DATA.address1;
                const change = TEST_DATA.changeAddress;
                const {appTx, ergoTx, uInputs} = new TxBuilder()
                    .input(from, txId, 0, '1000000000')
                    .dataInput(from.address, txId, 0)
                    .output(to.address, '100000000', null, '1000')
                    .fee('1000000')
                    .change(change)
                    .build();
                const tokenId = uInputs[0].box_id().to_str().toUpperCase();
                const tokensOutFlow = [
                    { header: 'Token [1]', body: ellipsize(test.model, tokenId) },
                    { header: 'Token [1] Value', body: '1000' }
                ];
                const tokensTxFlow = [
                    { header: 'Token [1]', body: ellipsize(test.model, tokenId) },
                    { header: 'Token [1] Value', body: 'Minting: 1000' }
                ];
                const expectedFlows = signTxFlows(test, auth, from, to, change, tokensOutFlow, tokensTxFlow);
                return { appTx, ergoTx, input: uInputs[0],
                         expectedFlows, flowsCount: expectedFlows.length };
            })
            .shouldSucceed(({flows, expectedFlows, ergoTx, input}, signatures) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(signatures).to.have.length(1);
                verifySignatures(ergoTx, signatures, input);
            })
            .run(({test, appTx}) => test.device.signTx(appTx, toNetwork(TEST_DATA.network)));

        authTokenFlows("can sign tx with few token ids")
            .init(async ({test, auth}) => {
                const from = TEST_DATA.address0;
                const to = TEST_DATA.address1;
                const change = TEST_DATA.changeAddress;
                const iTokenId = '1111111111111111111111111111111111111111111111111111111111111111';
                const iTokens = [{id: iTokenId, amount: '1234'}];
                const {appTx, ergoTx, uInputs} = new TxBuilder()
                    .input(from, txId, 0, '1000000000', iTokens)
                    .dataInput(from.address, txId, 0)
                    .output(to.address, '100000000', null, '5678')
                    .fee('1000000')
                    .change(change)
                    .burn(iTokens)
                    .build();
                const oTokenId = uInputs[0].box_id().to_str().toUpperCase();
                const tokensOutFlow = [
                    { header: 'Token [1]', body: ellipsize(test.model, oTokenId) },
                    { header: 'Token [1] Value', body: '5678' }
                ];
                const tokensTxFlow = [
                    { header: 'Token [1]', body: ellipsize(test.model, oTokenId) },
                    { header: 'Token [1] Value', body: 'Minting: 5678' },
                    { header: 'Token [2]', body: ellipsize(test.model, iTokenId) },
                    { header: 'Token [2] Value', body: 'Burning: 1234' }
                ];
                const expectedFlows = signTxFlows(test, auth, from, to, change, tokensOutFlow, tokensTxFlow);
                return { appTx, ergoTx, input: uInputs[0],
                         expectedFlows, flowsCount: expectedFlows.length };
            })
            .shouldSucceed(({flows, expectedFlows, ergoTx, input}, signatures) => {
                expect(flows).to.be.deep.equal(expectedFlows);
                expect(signatures).to.have.length(1);
                verifySignatures(ergoTx, signatures, input);
            })
            .run(({test, appTx}) => test.device.signTx(appTx, toNetwork(TEST_DATA.network)));
    });
});
