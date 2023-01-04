# Messaging protocol (Application Protocol Data Unit)

The communication protocol used by [BOLOS](https://ledger.readthedocs.io/en/latest/bolos/overview.html) to exchange [APDU](https://en.wikipedia.org/wiki/Smart_card_application_protocol_data_unit) is very close to [ISO 7816-4](https://www.iso.org/standard/77180.html) with a few differences:

- `Lc` length is always exactly 1 byte
- No `Le` field in APDU command
- Maximum size of APDU command is 260 bytes: 5 bytes of header + 255 bytes of data
- Maximum size of APDU response is 260 bytes: 258 bytes of response data + 2 bytes of status word

Status words tend to be similar to common [APDU responses](https://www.eftlab.com/knowledge-base/complete-list-of-apdu-responses/) in the industry.

## Command APDU

| Field | CLA | INS | P1 | P2 | Lc | Data | Le |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Size (B) | 1 | 1 | 1 | 1 | 1 | variable | 0 |

Where:
* **CLA=0xE0** is the APDU class number. As we don’t adhere to the strict APDU protocol, we can arbitrarily choose a value belonging to the "proprietary structure and coding of command/response" CLA range (0xD0-0xFE range)
* **INS** is the instruction number
* **P1** and **P2** are instruction parameters
* **Lc** is the length of the data body encoded as uint8. Note: unlike standard APDU, ledger.js produces **Lc** of exactly 1 byte (even for empty data). Data of length >= 256 are not supported by ledger.js
* **Data** is binary data
* **Le** is the max length of response. This APDU field is not present in ledger.js implementation

Upon receiving the APDU message, the Ledger Application checks:
* RX size >= 5 (i.e., the request has all required APDU fields)
* **CLA** is a valid CLA of the Ledger Ergo Application (**0xE0**)
* **INS** is a valid and enabled instruction
* **Lc** is consistent with RX, i.e. **Lc** + 5 == RX
* **INS** is not changed in the middle of the multi-APDU exchange.

All unused parameters (**P1** and **P2**, **Lc**) should be set to 0. Meaningful values are never 0 for any of the fields (i.e. enum values must start from 1).

## Response APDU

| Field | Data | SW1 | SW2 |
| --- | --- | --- | --- |
| Size (B) | variable | 1 | 1 |

where **SW1** and **SW2** represent the return code.

Known codes are:
* **0x9000** = OK (SW1=0x90, SW2=0x00)
* for error codes check [sw.h](../src/sw.h) file.

