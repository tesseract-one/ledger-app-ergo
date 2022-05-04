# Ergo Ledger App Communication Protocol

This document describes the communication protocol between Wallets/dApps and Ergo Ledger Application. It contains detailed information about messaging layer protocol, and available methods, which can be used on Ledger Ergo Application.

## Helper Libraries

This documentation is a pretty low-level one and created for library developers.

If you are Wallet or dApp developer it's simpler to use one of the helper libraries:

* [ledger-ergo-js](https://www.npmjs.com/package/ledger-ergo-js) - JavaScript helper library

## Message data format

Ergo Communication Protocol uses an envelope message defined by APDU protocol and a set of wrapped messages for actions that can be performed by the application.

For more information about request and response formats read [APDU](APDU.md) document.

## Application Instruction Codes

There is the list of the instructions which can be called in the Ergo Ledger Application:

* **0x01-0x0F** - General application status
    * [0x01 - Get Ledger Application version](INS-01-APP-VERSION.md)
    * [0x02 - Get Ledger Application name](INS-02-APP-NAME.md)
* **0x10-0x1F** - Public key / Addresses
    * [0x10 - Get the extended public key](INS-10-EXT-PUB-KEY.md)
    * [0x11 - Derive address](INS-11-DERIVE-ADDR.md)
* **0x20-0x2F** - Signing
    * [0x20 - Attest Input Box](INS-20-ATTEST-BOX.md)
    * [0x21 - Sign transaction](INS-21-SIGN-TRANSACTION.md)
