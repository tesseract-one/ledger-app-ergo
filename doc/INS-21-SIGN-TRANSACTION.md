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

#### Request
| INS | P1 | P2 | Lc | Data |
| --- | --- | --- | --- | --- |
| 0x21 | 0x01 | 0x01 - without token <br> 0x02 - with token | variable | see below |