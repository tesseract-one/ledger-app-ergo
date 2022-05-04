# 0x10 - Get the extended public key

Returns a combination of chain code and public key bytes for provided BIP44 path. An optional token can be added to create an authenticated session within the Ledger Application.

## Command

| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x10 | 0x01 - without token <br> 0x02 - with token |0x00 | variable | see below |

### Data
| Field | Size (B) | Description |
| --- | --- | --- | 
| BIP32 path length | 1 | Value: 0x02-0x0A (2-10). Count of path components |
| First derivation index | 4 | Big-endian. Value: 44' |
| Second derivation index | 4 | Big-endian. Value: 429’ (Ergo coin id) |
| [Optional] Third index | 4 | Big-endian. Any valid bip44 hardened value. |
| ... | 4 | ... |
| [Optional] Last index | 4 | Big-endian. Any valid bip44 value. |
| [Optional] Auth Token | 4 | Big-endian. Randomly generated value. Becomes an ID for the session for future use, i.e. once transaction signing is requested. If present, **P1** should be set to 0x02 |


Ledger Application checks all input parameters to be valid. Data length is also checked based on the **P1** parameter and **bip32 path length**. Ledger Application asks user permission to send extended public key information back. If Authorization Token is present, it’s presented to the user in the HEX format. Last Authorization Token is saved in RAM of the Ledger as current Application Session Token.

## Response

Response is 65 bytes of data which consists of 33 bytes of compressed public key and 32 bytes of chain code.

| Bytes [0-32] | Bytes [33-64] |
| --- | --- |
| Compressed Public Key | Chain Code |
