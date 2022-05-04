# 0x11 - Derive address

Derive the address for a given BIP44 path and return or show it to the user. This call is intended for address verification purposes (i.e., matching the address on Ledger with the one on the dApp/Wallet screen).

The address can’t be that of an account, or of external/internal address chain root, i.e. it needs to have:
* path_len >= 5,
* path[2] is hardened (account), and
* path[3] in [0,1] (internal/external chain)

## Command

| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x11 | 0x01 - return <br> 0x02 - display | 0x01 - without token <br> 0x02 - with token | variable | see below |

### Data
| Field | Size (B) | Description |
| --- | --- | --- | 
| Network Type | 1 | Value: 0x00-0xFC (0-252). Network Type |
| BIP32 path length | 1 | Value: 0x05-0x0A (5-10). Count of path components |
| First derivation index | 4 | Big-endian. Value: 44’ |
| Second derivation index | 4 | Big-endian. Value: 429’ (Ergo coin id) |
| Third index | 4 | Big-endian. Any valid bip44 hardened value. |
| ... | 4 | ... |
| [Optional] Last index | 4 | Big-endian. Any valid bip44 value. |
| [Optional] Auth Token | 4 | Big-endian. Randomly generated value (session). If present **P2** should be set to 0x02 |

## Response

Empty if "display" was sent. 38 bytes of the address data otherwise.
