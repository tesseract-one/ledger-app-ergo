const { assert, expect } = require('chai')
    .use(require('chai-bytes'));
const { Transaction, TxId, Tokens, Token, TokenId, TokenAmount, I64 } = require('ergo-lib-wasm-nodejs');
const { toNetwork, getApplication, removeMasterNode } = require('./helpers/common');
const { TEST_DATA } = require('./helpers/data');
const { AuthTokenFlows } = require('./helpers/flow');
const { UnsignedTransactionBuilder } = require('./helpers/transaction');

const signTxFlowCount = [3, 2];

function signTxFlows(device, auth, address, tokens = false) {
    const flows = [
        [
            { header: null, body: 'Confirm Attest Input' }
        ],
        [
            { header: null, body: 'Start P2PK signing' },
            { header: 'Path', body: removeMasterNode(address.path.toString()) }
        ],
        [
            { header: null, body: 'Confirm Transaction' },
            { header: 'P2PK Path', body: removeMasterNode(address.path.toString()) },
            { header: 'Transaction Amount', body: '1.000000000' },
            { header: 'Transaction Fee', body: '0.000000000' }
        ]
    ];
    if (tokens) {
        flows[2].push({ header: 'Token [1]', body: '29d2S7v...TmBhfV2' });
        flows[2].push({ header: 'Token [1]', body: 'T: 1000' });
    }
    if (auth) {
        flows[0].push({ header: 'Application', body: getApplication(device) });
        flows.splice(1, 1);
    }
    return flows;
};

describe("Transaction Tests", function () {
    context("Transaction Commands", function () {
        it("can attest input", async function () {
            this.timeout(5000);
            const attestInputFlow = auth => {
                const flow = [ { header: null, body: 'Confirm Attest Input' } ];
                if (auth) {
                    flow.push({ header: 'Application', body: getApplication(this.device) });
                }
                return flow;
            };
            const transaction = new UnsignedTransactionBuilder()
                .input(TEST_DATA.address0, TxId.zero(), 0)
                .build();
            const unsignedBox = transaction.inputs[0];
            await new AuthTokenFlows(this.device, this.screens).do(
                () => this.device.attestInput(unsignedBox),
                (attestedBox, auth, [flow]) => {
                    expect(flow).to.be.deep.equal(attestInputFlow(auth));
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
                },
                error => assert.fail(error)
            );
        });

        it("can sign tx", async function () {
            this.timeout(10000);
            const address = TEST_DATA.address0;
            const builder = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0)
                .dataInput(address.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress);
            const unsignedTransaction = builder.build();
            await new AuthTokenFlows(this.device, this.screens, signTxFlowCount).do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                (signatures, auth, flows) => {
                    expect(flows).to.be.deep.equal(signTxFlows(this.device, auth, address));
                    expect(signatures).to.have.length(1);
                    const unsigned = builder.buildErgo();
                    const signed = Transaction.from_unsigned_tx(unsigned, signatures);
                    // TODO verify signatures
                },
                error => assert.fail(error)
            );
        });

        it("can sign tx with zero data inputs", async function () {
            this.timeout(10000);
            const address = TEST_DATA.address0;
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress)
                .build();
            await new AuthTokenFlows(this.device, this.screens, signTxFlowCount).do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                (signatures, auth, flows) => {
                    expect(flows).to.be.deep.equal(signTxFlows(this.device, auth, address));
                    expect(signatures).to.have.length(1);
                },
                error => assert.fail(error)
            );
        });

        it("can not sign tx with zero inputs", async function () {
            this.timeout(10000);
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .dataInput(TEST_DATA.address0.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address)
                .change(TEST_DATA.changeAddress)
                .build();
            await new AuthTokenFlows(this.device, this.screens, [0, 0]).do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                (signatures, _, flows) => {
                    expect(flows).to.be.empty;
                    expect(signatures).to.be.empty;
                },
                error => assert.fail(error)
            );
        });

        it("can not sign tx with zero outputs", async function () {
            this.timeout(10000);
            const address = TEST_DATA.address0;
            const signTxNoOutputsFlows = (auth, address) => {
                const flows = [
                    [
                        { header: null, body: 'Confirm Attest Input' }
                    ],
                    [
                        { header: null, body: 'Start P2PK signing' },
                        { header: 'Path', body: removeMasterNode(address.path.toString()) }
                    ]
                ];
                if (auth) {
                    flows[0].push({ header: 'Application', body: getApplication(this.device) });
                    flows.splice(1, 1);
                }
                return flows;
            };
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0)
                .dataInput(address.address, TxId.zero(), 0)
                .change(TEST_DATA.changeAddress)
                .build();
            await new AuthTokenFlows(this.device, this.screens, [2, 1]).do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                signatures => expect(signatures).to.not.exist,
                (error, auth, flows) => {
                    expect(flows).to.be.deep.equal(signTxNoOutputsFlows(auth, address));
                    expect(error).to.be.an('error');
                    expect(error.name).to.be.equal('DeviceError');
                    expect(error.message).to.be.equal('Bad output count');
                }
            );
        });

        it("can sign tx with tokens", async function () {
            this.timeout(10000);
            const address = TEST_DATA.address0;
            const tokens = new Tokens();
            const tokenId = TokenId.from_str('1111111111111111111111111111111111111111111111111111111111111111');
            tokens.add(new Token(tokenId, TokenAmount.from_i64(I64.from_str('1000'))));
            const unsignedTransaction = new UnsignedTransactionBuilder()
                .input(address, TxId.zero(), 0, tokens)
                .dataInput(address.address, TxId.zero(), 0)
                .output('100000000', TEST_DATA.address1.address, tokens)
                .change(TEST_DATA.changeAddress)
                .tokenId(tokenId.as_bytes())
                .build();
            await new AuthTokenFlows(this.device, this.screens, signTxFlowCount).do(
                () => this.device.signTx(unsignedTransaction, toNetwork(TEST_DATA.network)),
                (signatures, auth, flows) => {
                    expect(flows).to.be.deep.equal(signTxFlows(this.device, auth, address, true));
                    expect(signatures).to.have.length(1);
                },
                error => assert.fail(error)
            );
        });
    });
});
