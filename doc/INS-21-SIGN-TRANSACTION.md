# 0x21 - Sign transaction

Provides the capability to sign a transaction through the specialized protocol that allows sending the transaction in chunks.

Splitting the transaction into chunks is preferred to sending a full raw transaction for several reasons:
* A transaction (or even its part) can get larger than limited Ledger hardware can handle
* Even if buffering was a viable option, such an approach is way faster on limited Ledger hardware
* Having no transaction parsing stage can be considered safer, eliminating a full extra processing stage (less potential bugs)

## [0x01-0x0F] - Start signing 
Set of methods to start the transaction signing process. Methods are mutually exclusive and the proper method should be called.

The app generates and returns a unique 1-byte identifier for the initialized session (Session ID), which is used in all calls related to this session.

If Authorization Token was previously used and matches the stored Application Session Token, the application will start signing session without additional user approval. If the token was never used before and isn’t a current Application Session Token, the Ledger Application must ask the user for confirmation. Regardless of having a token or not, proceeding further with this step does NOT mean the transaction is going to be signed. This is rather an additional security measure to identify clients that have never interacted with the application before.

To avoid any confusion, Authorization Token and Session ID are two different values and should not be mixed up.

### 0x01 - Start P2PK signing
Starts the P2PK transaction signing process for the private key with provided BIP44 path.

Ledger Application is clearing all internal buffers and preparing for transaction signing. The BIP44 path is stored inside the App.

#### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x01 | 0x01 - without token <br> 0x02 - with token | variable | see below |

##### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| BIP44 path length | 1 | Value: 0x02-0x0A (2-10). The number of path components |
| First derivation index | 4 | Big-endian. Value: 44’ |
| Second derivation index | 4 | Big-endian. Value: 429’ (Ergo coin id) |
| [Optional] Third index | 4 | Big-endian. Any valid bip44 hardened value. |
| ... | 4 | ... |
| [Optional] Last index | 4 | Big-endian. Any valid bip44 value. |
| [Optional] Authorization Token | 4 | Optional authorization token. |

#### Response

Returns one byte - **Session ID** in range [1-255]. This session id should be sent as **P2** parameter to other calls.

## 0x10 - Start Transaction data
Starts transaction uploading process. Sets the number of Inputs and Outputs in the Transaction.

Ledger Application is clearing all internal buffers and preparing for transaction uploading. The number of Inputs, Data Inputs, Token Ids, and Outputs are stored inside the App.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x10 | Session ID | 0x07 | see below |

#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| Number of TX Inputs | 2 | Big-endian. |
| Number of TX Data Inputs | 2 | Big-endian |
| Number of TX Distinct Token Ids | 1 | Number of unique token ids in TX |
| Number of TX Outputs | 2 | Big-endian. |

## 0x11 - Add Token Ids
This call adds Distinct Token Ids to the transaction. Up to 7 Ids can be added in a single call. The hard limit of tokens in a single transaction is 10.
Must be called only if “Start Trasnaction data” call had “Number of TX Distinct Token Ids “ param set greater than zero. Must be called before setting Inputs.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x11 | Session ID | variable | see below |

#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| Token Id 1 | 32 | First token Id |
| [Optional] Token Id 2 | 32 | Second token Id |
| ... | 32 | ... |
| [Optional] Token Id 7 | 32 | Seventh token Id |

## 0x12 - Add Input Box frame
This call adds Input Box to the current TX. It must be called after calling the “Start Trasnaction data” or “Token Ids” method. All inputs must be set before calling any other method. Inputs must be sent in the same order as they are in the original transaction.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x12 | Session ID | variable | see below |
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
| Context Extension Length | 4 | Big-endian. Length of serialized context extension in bytes. Can be 0, if context-extension is empty. Can be sent only with Frame 0. |

## 0x13 - Add Input Box Context Extension chunk
This call adds Context Extension bytes to the current Input Box. It must be called after calling the “Input Box Frames” method. The call can be omitted if the context extension is empty and its length is set to 0 in a previous call.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x13 | Session ID | variable | chunk bytes < 256b |

## 0x14 - Add Data Inputs
This call adds Data Inputs to the current TX. It must be called after calling the “Input Box” method. All inputs must be set before calling any other method. Inputs must be sent in the same order as they are in the original transaction. Up to 7 Ids can be added in a single call.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x14 | Session ID | variable | see below |

#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| Box ID 1 | 32 | First Data Input Id |
| [Optional] Box ID 2 | 32 | Second Data Input Id |
| ... | 32 | ... |
| [Optional] Box ID 7 | 32 | Seventh Data Input Id |

## 0x15 - Add Output Box: Start
This call starts the process of adding an Output Box to TX. It must be called after all Inputs and Token IDs are added. All Outputs must be set before any other calls.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x15 | Session ID | 0x15 | see below |
#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| Value | 8 | Box value. Big-endian. |
| Ergo Tree Size | 4 | Size in bytes of Ergo Tree data (0 for Change Tree or Miners Fee Tree). Big-endian. |
| Creation Height | 4 | Big-endian |
| Tokens Count | 1 | Tokens count inside Box |
| Additional Registers Size | 4 | Size in bytes of serialized Additional Registers. Can be 0 if registers are empty. Big-endian. |

## 0x16 - Add Output Box: Ergo Tree chunk
Adds serialized Ergo Tree chunk to the current Output Box. Can be called if the “Ergo Tree Size” of is not 0.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x16 | Session ID | variable | chunk bytes < 256b |

## 0x17 - Add Output Box: Miners Fee tree
Add Miners Fee tree to the current Output Box. Can be called only if “Ergo Tree Size” is 0.

If you have a custom network, where Miners Fee tree is different from the test net and the main net you should put it as Ergo Tree chunks instead.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x17 | Session ID | 0x01 | 0x01 - main net <br> 0x02 - test net |

## 0x18 - Add Output Box: Change tree
Add Change tree to the current Output Box with provided BIP44 path. Can be called only if “Ergo Tree Size” is 0.
### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x18 | Session ID | variable | see below |

#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| BIP44 path length | 1 | Value: 0x02-0x0A (2-10). The number of path components |
| First derivation index | 4 | Big-endian. Value: 44’ |
| Second derivation index | 4 | Big-endian. Value: 429’ (Ergo coin id) |
| [Optional] Third index | 4 | Big-endian. Any valid bip44 hardened value. |
| ... | 4 | ... |
| [Optional] Last index | 4 | Big-endian. Any valid bip44 value. |

## 0x19 - Add Output Box: Tokens
Add Tokens to the current Output Box. Can be used only when Transaction has Distinct Token Ids.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x19 | Session ID | variable | see below |

#### Data
| Field | Size (B) | Description |
| --- | --- | --- |
| Token 1 Index | 4 | Big-endian. Token 1 Index in Distinct Token Ids. |
| Token 1 Value | 8 | Big-endian. Token 1 value |
| ... | ... | ... |

## 0x1A - Add Output Box: Registers chunk
Add serialized Registers data chunk to the Output Box. Can be called only if Additional Registers size > 0.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x1A | Session ID | variable | chunk bytes  < 256b |

## 0x20 - Confirm and Sign
Notifies the Ledger Application that all the data is sent and requests the user’s approval to proceed with the signing operation. At this stage, the application displays submitted transaction info and ask the user to check if the transaction data presented on the screen is correct. If the user confirms - the application signs the uploaded transaction with the initialized method and returns the signature.

### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x20 | Session ID | 0x00 | empty |

### Response
56 bytes of the Signature.
