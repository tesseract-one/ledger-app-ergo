import binascii

TRANSACTIONS = [
    {
        "id": "0f51ac8bd9c7413ea9a6ceb1d67688f1786dd43f6bb71b9715e9ff0ebda61136",
        "tokens": [
            "b979c439dc698ce5e8bbb21c722a6e23721af010e4df8c72de0bfd0c3d9ccf6b",
            "e56847ed19b3dddb72828fcfb992fdf7310828cf291221269b7ffc72fd66706e",
            "e56847ed12b3dddb72828fcfb992fdf7310828cf291221269b7ffc72fd66706e",
            "e56847ed12b3dddb72828fcfb992fdf7310828cf291221269b7aac72fd66706e"
        ],
        "outputs": [
            {
                "index": 0,
                "value": 74187765000000000,
                "height": 284761,
                "tree": "101004020e36100204a00b08cd0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798ea02d192a39a8cc7a7017300730110010204020404040004c0fd4f05808c82f5f6030580b8c9e5ae040580f882ad16040204c0944004c0f407040004000580f882ad16d19683030191a38cc7a7019683020193c2b2a57300007473017302830108cdeeac93a38cc7b2a573030001978302019683040193b1a5730493c2a7c2b2a573050093958fa3730673079973089c73097e9a730a9d99a3730b730c0599c1a7c1b2a5730d00938cc7b2a5730e0001a390c1a7730f",
                "tokens": [
                    {"index": 0, "value": 67500123000},
                    {"index": 1, "value": 12300675000},
                ],
                "registers": "03070327e65711a59378c59359c3e1d0f7abe906479eccb76094e50fe79d743ccc15e60e20e26d41ed030a30cd563681e72f0b9c07825ac983f8c253a87a43c1da21958ece05feaff5de0f"
            },
            {
                "index": 1,
                "value": 67500000000,
                "height": 284761,
                "tree": "100204a00b08cd021dde34603426402615658f1d970cfa7c7bd92ac81a8b16eeebff264d59ce4604ea02d192a39a8cc7a70173007301",
                "tokens": [
                    {"index": 1, "value": 32100675000},
                    {"index": 2, "value": 300000},
                    {"index": 3, "value": 123456789}
                ],
                "registers": "02070327e65711a59378c59359c3e1d0f7abe906479eccb76094e50fe79d743ccc15e60e20e26d41ed030a30cd563681e72f0b9c07825ac983f8c253a87a43c1da21958ece"
            }
        ]
    }
]


def serialize_box_header(info: dict, tx_id: str) -> bytes:
    return binascii.unhexlify(tx_id) + \
        info["index"].to_bytes(2, 'big', signed=False) + \
        info["value"].to_bytes(8, 'big', signed=False) + \
        int(len(info["tree"]) / 2).to_bytes(4, 'big', signed=False) + \
        info["height"].to_bytes(4, 'big', signed=False) + \
        len(info["tokens"]).to_bytes(1, 'big', signed=False) + \
        int(len(info["registers"]) / 2).to_bytes(4, 'big', signed=False)


def serialize_box_header_sign(info: dict) -> bytes:
    return info["value"].to_bytes(8, 'big', signed=False) + \
        int(len(info["tree"]) / 2).to_bytes(4, 'big', signed=False) + \
        info["height"].to_bytes(4, 'big', signed=False) + \
        len(info["tokens"]).to_bytes(1, 'big', signed=False) + \
        int(len(info["registers"]) / 2).to_bytes(4, 'big', signed=False)


def serialize_tx_tokens(tokens: list) -> bytes:
    result = bytes()
    for token in tokens:
        result += binascii.unhexlify(token)
    return result


def serialize_box_tokens(box_tokens: list, tx_tokens: list) -> bytes:
    result = bytes()
    for token in box_tokens:
        result += binascii.unhexlify(tx_tokens[token["index"]])
        result += token["value"].to_bytes(8, 'big', signed=False)
    return result


def serialize_box_tokens_sign(tokens: list) -> bytes:
    result = bytes()
    for token in tokens:
        result += token["index"].to_bytes(4, 'big', signed=False)
        result += token["value"].to_bytes(8, 'big', signed=False)
    return result


def print_input_frame(frame: bytes):
    offset = 0
    print(f"Frame:")
    print(f"  ID: {binascii.hexlify(frame[offset:offset+32]).decode()}")
    offset += 32
    print(f"  Count: {frame[offset]}")
    offset += 1
    print(f"  Index: {frame[offset]}")
    offset += 1
    value = int.from_bytes(frame[offset:offset+8], 'big', signed=False)
    offset += 8
    print(f"  Value: {value}")
    token_count = int(frame[offset])
    offset += 1
    print(f"  Tokens[{token_count}]:")
    for token in range(0, token_count):
        id = frame[offset:offset+32]
        offset += 32
        value = int.from_bytes(frame[offset:offset+8], 'big', signed=False)
        offset += 8
        print(f"    [{token}] Id: {binascii.hexlify(id).decode()}")
        print(f"    [{token}] Value: {value}")
    print(f"  Signature: {binascii.hexlify(frame[offset:]).decode()}")
