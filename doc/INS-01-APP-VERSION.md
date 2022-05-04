# 0x01 - Get Ledger Application Version
Returns the Ledger Application version.

## Command
| INS | P1 | P2 | Lc |
| --- | --- | --- | --- |
| 0x01 | 0x00 | 0x00 | 0x00 |

Ledger Application checks that **P1**, **P2**, and **Lc** are all set to **0x00**. If not - the error code is returned.

## Response

| Byte 0 | Byte 1 | Byte 2 | Byte 3 |
| --- | --- | --- | --- |
| Major version | Minor version | Patch | Debug |

**Debug** flag is set to 0x01 for all debug builds of the Ledger Application. Apps and Wallets should show an error message when this flag is set (debug version of Ledger Application should never be used in any real environment).
