# 0x02 - Get Ledger Application Name
Returns the Ledger Application name.

## Command
| INS | P1 | P2 | Lc |
| --- | --- | --- | --- |
| 0x02 | 0x00 | 0x00 | 0x00 |

Ledger Application checks that **P1**, **P2**, and **Lc** are all set to **0x00**. If not - the error code is returned.

## Response

Respone of this call is an array of ANSI encoded characters with application name ("Ergo").
