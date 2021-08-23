import argparse
import binascii
import random
from typing import List, Optional

from ledgerblue.comm import getDongle
from test_cmds_txs import *

CLA = 0xE0
BIP32_HARD_PREFIX = 0x80000000

application_token = random.randint(
    1, 0xFFFFFFFE).to_bytes(4, 'big', signed=False)

ledger = getDongle()

cli = argparse.ArgumentParser()
subparsers = cli.add_subparsers(dest="subcommand")


def argument(*name_or_flags, **kwargs):
    return ([*name_or_flags], kwargs)


def subcommand(args=[], parent=subparsers):
    def decorator(func):
        parser = parent.add_parser(func.__name__, description=func.__doc__)
        for arg in args:
            parser.add_argument(*arg[0], **arg[1])
        parser.set_defaults(func=func)
    return decorator


def ledger_cmd(cmd: int, p1: int, p2: int, data: bytes):
    command = bytes([CLA, cmd & 0xFF, p1 & 0xFF, p2 &
                    0xFF, len(data) & 0xFF]) + data
    return ledger.exchange(command)


def ledger_upload_data(cmd: int, p1: int, p2: int, data: bytes):
    chunks = [data[i:i+255] for i in range(0, len(data), 255)]
    result = []
    for chunk in chunks:
        result.append(ledger_cmd(cmd, p1, p2, chunk))
    return result


def bip44(account: int, change: Optional[int] = None, address: Optional[int] = None) -> bytes:
    length = 3
    path = (BIP32_HARD_PREFIX + 44).to_bytes(4, 'big') + \
        (BIP32_HARD_PREFIX + 429).to_bytes(4, 'big') + \
        (BIP32_HARD_PREFIX + account).to_bytes(4, 'big')
    if change is not None:
        path += change.to_bytes(4, 'big')
        length += 1
    if address is not None:
        path += address.to_bytes(4, 'big')
        length += 1
    return bytes([length]) + path


def attest_box(tx_index: int, box_index: int) -> List[bytes]:
    tx = TRANSACTIONS[tx_index]
    box = tx["outputs"][box_index]
    tokens = tx["tokens"]

    tx_header = serialize_tx_header(
        tx["prefix"], tx["suffix"], len(tokens))
    tx_header += application_token
    session_id = ledger_cmd(0x20, 0x01, 0x02, tx_header)[0]

    prefix = binascii.unhexlify(tx["prefix"])
    ledger_upload_data(0x20, 0x02, session_id, prefix)

    token_chunks = [tokens[i:i+7] for i in range(0, len(tokens), 7)]

    for chunk in token_chunks:
        token_data = serialize_tx_tokens(chunk)
        ledger_cmd(0x20, 0x03, session_id, token_data)

    suffix = binascii.unhexlify(tx["suffix"])
    ledger_upload_data(0x20, 0x04, session_id, suffix)

    box_header = serialize_box_header(box)
    ledger_cmd(0x20, 0x05, session_id, box_header)

    box_tree = binascii.unhexlify(box["tree"])
    ledger_upload_data(0x20, 0x06, session_id, box_tree)

    box_tokens = serialize_box_tokens(box["tokens"])
    ledger_cmd(0x20, 0x07, session_id, box_tokens)

    frames = 0
    for register in box["registers"]:
        data = binascii.unhexlify(register)
        res = ledger_cmd(0x20, 0x08, session_id, data)
        frames = res[0] if len(res) > 0 else 0
    response = []
    for frame in range(0, frames):
        data = ledger_cmd(0x20, 0x09, session_id, bytes([frame]))
        response.append(data)
    return response


@subcommand()
def name(args: argparse.Namespace):
    name = ledger_cmd(0x02, 0x00, 0x00, bytes())
    print(f"Name: {name.decode('ascii')}\n")


@subcommand()
def version(args: argparse.Namespace):
    version = ledger_cmd(0x01, 0x00, 0x00, bytes())
    print(f"Version: {version[0]}.{version[1]}.{version[2]}\n")


@subcommand([argument("account", type=int, nargs="?", default=0)])
def extpubkey(args: argparse.Namespace):
    path = bip44(args.account)
    path += application_token
    key = ledger_cmd(0x10, 0x02, 0x00, path)
    print(binascii.hexlify(key).decode())


@subcommand([argument("account", type=int, nargs="?", default=0),
             argument("change", type=int, nargs="?", default=0),
             argument("address", type=int, nargs="?", default=0),
             argument("--net", type=int, nargs="?", default=0)])
def getaddr(args: argparse.Namespace):
    path = bip44(args.account, args.change, args.address)
    path += application_token
    address = ledger_cmd(0x11, 0x01, 0x02, bytes([args.net & 0xFF]) + path)
    print("Address: " + binascii.hexlify(address).decode())


@subcommand([argument("account", type=int, nargs="?", default=0),
             argument("change", type=int, nargs="?", default=0),
             argument("address", type=int, nargs="?", default=0),
             argument("--net", type=int, nargs="?", default=0)])
def showaddr(args: argparse.Namespace):
    path = bip44(args.account, args.change, args.address)
    path += application_token
    ledger_cmd(0x11, 0x02, 0x02, bytes([args.net & 0xFF]) + path)


@subcommand([argument("tx", type=int, nargs="?", default=0),
             argument("box", type=int, nargs="?", default=0)])
def attest(args: argparse.Namespace):
    frames = attest_box(args.tx, args.box)
    for frame in frames:
        print_input_frame(frame)


@subcommand([argument("tx", type=int, nargs="?", default=0),
             argument("box", type=int, nargs="?", default=0)])
def sign(args: argparse.Namespace):
    frames = attest_box(args.tx, args.box)
    tx = TRANSACTIONS[args.tx]
    box = tx["outputs"][args.box]

    tokens = tx["tokens"]

    data = int(1).to_bytes(2, "big", signed=False) + \
        int(0).to_bytes(2, "big", signed=False) + \
        len(tokens).to_bytes(1, "big", signed=False) + \
        int(1).to_bytes(2, "big", signed=False) + application_token
    session_id = ledger_cmd(0x21, 0x01, 0x02, data)[0]

    token_chunks = [tokens[i:i+7] for i in range(0, len(tokens), 7)]

    for chunk in token_chunks:
        token_data = serialize_tx_tokens(chunk)
        ledger_cmd(0x21, 0x02, session_id, token_data)

    ledger_cmd(0x21, 0x03, session_id,
               frames[0] + int(0).to_bytes(4, "big", signed=False))

    box_header = serialize_box_header_sign(box)
    ledger_cmd(0x21, 0x06, session_id, box_header)

    box_tree = binascii.unhexlify(box["tree"])
    ledger_upload_data(0x21, 0x07, session_id, box_tree)

    box_tokens = serialize_box_tokens(box["tokens"])
    ledger_cmd(0x21, 0x0A, session_id, box_tokens)

    for register in box["registers"]:
        data = binascii.unhexlify(register)
        ledger_cmd(0x21, 0x0B, session_id, data)

    tx_id = ledger_cmd(0x21, 0x0C, session_id, box_tokens)
    print("TXID: " + binascii.hexlify(tx_id).decode())

    account = bip44(0, 0, 0)
    signature = ledger_cmd(0x21, 0x0D, session_id, account)
    print("Signature: " + binascii.hexlify(signature).decode())


if __name__ == "__main__":
    args = cli.parse_args()
    if args.subcommand is None:
        cli.print_help()
    else:
        args.func(args)
