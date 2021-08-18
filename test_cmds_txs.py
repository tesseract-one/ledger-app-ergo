import binascii
from typing import Sized

TRANSACTIONS = [
    {
        "prefix": (
            "01" +  # inputs count
            # input 0 id
            "9126af0675056b80d1fda7af9bf658464dbfa0b128afca7bf7dae18c27fe8456" +
            "0000" +  # input 0 proof
            "01" +  # data_input count
            # data input 0 id
            "e26d41ed030a30cd563681e72f0b9c07825acbb3f8c253a87a43c1da21958ece"
        ),
        "tokens": [
            "b979c439dc698ce5e8bbb21c722a6e23721af010e4df8c72de0bfd0c3d9ccf6b",
            "e56847ed19b3dddb72828fcfb992fdf7310828cf291221269b7ffc72fd66706e",
            "e56847ed12b3dddb72828fcfb992fdf7310828cf291221269b7ffc72fd66706e",
            "e56847ed12b3dddb72828fcfb992fdf7310828cf291221269b7aac72fd66706e"
        ],
        "suffix": (
            "02" +  # outputs count
            # output 0
            "80a4859c91ace48301" +  # output 0 value
            # output 0 tree
            "101004020e36100204a00b08cd0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798ea02d192a39a8cc7a7017300730110010204020404040004c0fd4f05808c82f5f6030580b8c9e5ae040580f882ad16040204c0944004c0f407040004000580f882ad16d19683030191a38cc7a7019683020193c2b2a57300007473017302830108cdeeac93a38cc7b2a573030001978302019683040193b1a5730493c2a7c2b2a573050093958fa3730673079973089c73097e9a730a9d99a3730b730c0599c1a7c1b2a5730d00938cc7b2a5730e0001a390c1a7730f" +
            "d9b011" +  # output 0 height
            "02" +  # tokens count
            "00" +  # token 0 index
            "f8c6c8bafb01" +  # token0 val
            "01" +  # token 1 index
            "b8cfb5e92d" +  # token 1 val
            "03" +  # registers count
            # register 4 val
            "070327e65711a59378c59359c3e1d0f7abe906479eccb76094e50fe79d743ccc15e6" +
            # register 5 val
            "0e20e26d41ed030a30cd563681e72f0b9c07825ac983f8c253a87a43c1da21958ece" +
            # register 6 val
            "05feaff5de0f" +
            # output 1
            "8086c1bafb01" +  # output 1 value
            # output 1 tree
            "100204a00b08cd021dde34603426402615658f1d970cfa7c7bd92ac81a8b16eeebff264d59ce4604ea02d192a39a8cc7a70173007301" +
            "d9b011" +  # output 1 height
            "03" +  # tokens count
            "01" +  # token 0 index
            "b8dbe5ca77" +  # token 0 val
            "02" +  # token 1 index
            "e0a712" +  # token 1 val
            "03" +  # token 2 index
            "959aef3a" +  # token 2 val
            "02" +  # registers count
            # register 4 value
            "070327e65711a59378c59359c3e1d0f7abe906479eccb76094e50fe79d743ccc15e6" +
            # register 5 value
            "0e20e26d41ed030a30cd563681e72f0b9c07825ac983f8c253a87a43c1da21958ece"
        ),
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
                "registers": [
                    "070327e65711a59378c59359c3e1d0f7abe906479eccb76094e50fe79d743ccc15e6",
                    "0e20e26d41ed030a30cd563681e72f0b9c07825ac983f8c253a87a43c1da21958ece",
                    "05feaff5de0f"
                ]},
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
                "registers": [
                    "070327e65711a59378c59359c3e1d0f7abe906479eccb76094e50fe79d743ccc15e6",
                    "0e20e26d41ed030a30cd563681e72f0b9c07825ac983f8c253a87a43c1da21958ece"
                ]
            }
        ]
    }
]


def serialize_tx_header(prefix: str, suffix: str, tokens_count: int) -> bytes:
    return int(len(prefix) / 2).to_bytes(4, 'big', signed=False) + \
        int(len(suffix) / 2).to_bytes(4, 'big', signed=False) + \
        tokens_count.to_bytes(1, 'big', signed=False)


def serialize_box_header(info: dict) -> bytes:
    return info["index"].to_bytes(2, 'big', signed=False) + \
        info["value"].to_bytes(8, 'big', signed=False) + \
        int(len(info["tree"]) / 2).to_bytes(4, 'big', signed=False) + \
        info["height"].to_bytes(4, 'big', signed=False) + \
        len(info["tokens"]).to_bytes(1, 'big', signed=False) + \
        len(info["registers"]).to_bytes(1, 'big', signed=False)


def serialize_tx_tokens(tokens: list) -> bytes:
    result = bytes()
    for token in tokens:
        result += binascii.unhexlify(token)
    return result


def serialize_box_tokens(tokens: list) -> bytes:
    result = bytes()
    for token in tokens:
        result += token["index"].to_bytes(4, 'big', signed=False)
        result += token["value"].to_bytes(8, 'big', signed=False)
    return result


def print_input_frame(index: int, frame: bytes):
    offset = 0
    print(f"Frame[{index}]:")
    print(f"  ID: {binascii.hexlify(frame[offset:offset+31]).decode()}")
    offset += 32
    print(f"  Index: {frame[offset]}")
    offset += 1
    token_count = int(frame[offset])
    offset += 1
    value = int.from_bytes(frame[offset:offset+8], 'big', signed=False)
    offset += 8
    print(f"  Value: {value}")
    print(f"  Tokens[{token_count}]:")
    for token in range(0, token_count):
        id = frame[offset:offset+32]
        offset += 32
        value = int.from_bytes(frame[offset:offset+8], 'big', signed=False)
        offset += 8
        print(f"    [{token}] Id: {binascii.hexlify(id).decode()}")
        print(f"    [{token}] Value: {value}")
    print(f"  Signature: {binascii.hexlify(frame[offset:]).decode()}")
