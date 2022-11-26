# 0x20 - Attest Input Box 
The purpose of this call is to return and attest ERG and other token amounts for a given Box.

This is related to bullet 2 of the Motivation section in the following [BIP-0143](https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki).

When signing a transaction, Box IDs provided as transaction inputs do not contain ERG or token amounts but these amounts are needed by Ledger to correctly calculate and display transaction fees. Therefore, Ledger must learn these amounts in another way.

A naive implementation could be to simply provide these amounts to Ledger without any verification. This however leads to a whole set of issues. For example:
* Unintentional dApp/Wallet malfunction - can be caused due to programming errors (i.e. integer overflow). In this case, a dApp/Wallet can potentially submit the wrong amounts. Can be especially common in JavaScript-based clients, due to the fact that the Number in JS is limited to 53bit int value, while the Ergo amount is a 64bit integer value.
* Intentional attack - an attacker presents false Box values to Ledger, potentially leading it to assume there are fewer (than actually present) tokens in the input and thus tricking users to unknowingly burn them.

To address such issues, Ledger Application implementation requires each transaction input to be verified before signing a transaction.

The amount of unique tokens inside the transaction is hard limited to **20**, due to limited memory resources availability on Ledger hardware.

## 0x01 - Box start

Start attestation process. Provide information about the Box.

The app generates and returns a unique 1-byte identifier for the initialized session (Session ID), which is used in all calls related to this session.

If Authorization Token was previously used and matches the stored Application Session Token, the application will start attestation session without additional user approval. If the token was never used before and isn’t a current Application Session Token, the Ledger Application must ask the user for confirmation. This is an additional security measure to identify clients that have never interacted with the application before.

To avoid any confusion, Authorization Token and Session ID are two different values and should not be mixed up.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x20 | 0x01 | 0x01 - without token <br> 0x02 - with token | 0x37/0x3B | see below |

#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| Transaction ID | 32 | ID of the Transaction containing this Box |
| Box Index | 2 | Box index in the Transaction. Big-endian |
| Value | 8 | Box value. Big-endian. |
| Ergo Tree Size | 4 | Size in bytes of serialized Ergo Tree data. Big-endian. |
| Creation Height | 4 | Big-endian |
| Tokens Count | 1 | Tokens count inside the Box |
| Additional Registers Size | 4 | Size in bytes of serialized Additional Registers data. Big-endian |
| [Optional] Auth Token | 4 | Big-endian. Randomly generated value. Becomes an ID for the session for future use, i.e. once transaction signing is requested. If present, **P2** should be set to 0x02 |

### Response

Returns one byte - **Session ID** in range [1-255]. This session id should be sent as **P2** parameter to other calls.

## 0x02 - Add Ergo Tree chunk

Add Ergo Tree bytes to the Box.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x20 | 0x02 | Session ID | variable | Ergo Tree chunk < 256b |

#### Data
A chunk of the Box Ergo Tree. The maximum size of the chunk is 255 bytes.

### Response

If Box is finished then 1 byte of the data (amount of frames) or empty if more data needed.

## 0x03 - Add Tokens

Add Tokens to the current Box. Can be used only when Box has token count > 0. Up to 6 tokens can be added in one call (message size should be less than 256 bytes).

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x20 | 0x03 | Session ID | variable | see below |

#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| Token 1 ID | 32 | Token 1 ID. |
| Token 1 Value | 8 | Big-endian. Token 1 value |
| ... | ... | ... |

### Response

If Box is finished then 1 byte of the data (amount of frames) or empty if more data needed.

## 0x04 - Add Registers chunk

Add Registers data chunk to the current Box. Can be used only when Box has additional registers size > 0.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x20 | 0x04 | Session ID | variable | Registers chunk < 256b |

#### Data
A chunk of the Box serialized Registers data. The maximum size of the chunk is 255 bytes.

### Response

If Box is finished then 1 byte of the data (amount of frames) or empty if more data needed.

## 0x05 - Get Attested Box frame

Get results of the attestation. Results returned in set of frames. All frames should be retrieved from the Ledger Application. Amount of frames will be returned from previous calls.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x20 | 0x05 | Session ID | 0x01 | Frame Index (from 0) |

### Response

Attested Box frame with amounts of tokens and box value, signed with a temporary Session Key.

#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| Box Id | 32 | Id of this Box |
| Frames Count | 1 | Frames count for this Box |
| Frame Index | 1 | Index of the frame. From 0. |
| Amount | 8 | Big-endian. Amount of ERG in this Box |
| Token Count | 1 | Amount of tokens in this frame |
| First Token Id | 32 | First token Id |
| First Token Amount | 8 | Big-endian. Amount of the first token in this Box |
| ... | ... | ... |
| [Optional] Fourth Token Id | 32 | Fourth token Id |
| [Optional] Fourth Token Amount | 8 | Big-endian. Amount of the fourth token in this Box |
| Attestation | 16 | HMAC(Id, FCount, FIndex, Amount, TCount, [TokenId: Amount], Session Key) |

Attestation is the first 16 bytes of HMAC_256 signature of **(Box Id, Frames Count, Frame Index, Amount, Token Count, [TokenId: Amount])** tuple with a temporary Session Key, which is randomly generated on each Ledger Application start.