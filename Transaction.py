import urllib.request
import json
from binascii import unhexlify, crc32

transaction_length = 64
# This is the URL where we will be taking our transaction information from, {0} is where the tx number will be placed
transaction_url = "https://blockchain.info/rawtx/{0}?format=json&scripts=true"

op_code = {0x0: "OP_0", 0x4c: "OP PUSHDATA1", 0x4d: "OP PUSHDATA2", 0x4e: "OP PUSHDATA4", 0x4f: "OP_1NEGATE",
           0x51: "OP_1", 0x52: "OP_2", 0x53: "OP_3", 0x54: "OP_4", 0x55: "OP_5", 0x56: "OP_6", 0x57: "OP_7",
           0x58: "OP_8", 0x59: "OP_9", 0x5a: "OP_10", 0x5b: "OP_11", 0x5c: "OP_12", 0x5d: "OP_13", 0x5e: "OP_14",
           0x5f: "OP_15", 0x60: "OP_16", 0x61: "OP_NOP", 0x63: "OP_IF", 0x64: "OP_NOTIF", 0x67: "OP_ELSE",
           0x68: "OP_ENDIF", 0x69: "OP_VERIFY", 0x6a: "OP_RETURN", 0x6b: "OP_TOALTSTACK", 0x6c: "OP_FROMALTSTACK",
           0x73: "OP_IFDUP", 0x74: "OP_DEPTH", 0x75: "OP_DROP", 0x76: "OP_DUP", 0x77: "OP_NIP", 0x78: "OP_OVER",
           0x79: "OP_PICK", 0x7a: "OP_ROLL", 0x7b: "OP_ROT", 0x7c: "OP_SWAP", 0x7d: "OP_TUCK", 0x6d: "OP_2DROP",
           0x6e: "OP_2DUP", 0x6f: "OP_3DUP", 0x70: "OP_2OVER", 0x71: "OP_2ROT", 0x72: "OP_2SWAP", 0x7e: "OP_CAT",
           0x7f: "OP_SUBSTR", 0x80: "OP_LEFT", 0x81: "OP_RIGHT", 0x82: "OP_SIZE", 0x83: "OP_INVERT", 0x84: "OP_AND",
           0x85: "OP_OR", 0x86: "OP_XOR", 0x87: "OP_EQUAL", 0x88: "OP_EQUALVERIFY", 0x8b: "OP_1ADD", 0x8c: "OP_1SUB",
           0x8d: "OP_2MUL", 0x8e: "OP_2DIV", 0x8f: "OP_NEGATE", 0x90: "OP_ABS", 0x91: "OP_NOT", 0x92: "OP_0NOTEQUAL",
           0x93: "OP_ADD", 0x94: "OP_SUB", 0x95: "OP_MUL", 0x96: "OP_DIV", 0x97: "OP_MOD", 0x98: "OP_LSHIFT",
           0x99: "OP_RSHIFT", 0x9a: "OP_BOOLAND", 0x9b: "OP_BOOLOR", 0x9c: "OP_NUMEQUAL", 0x9d: "OP_NUMEQUALVERIFY",
           0x9e: "OP_NUMNOTEQUAL", 0x9d: "OP_NUMEQUALVERIFY", 0x9e: "OP_NUMNOTEQUAL", 0x9f: "OP_LESSTHAN",
           0xa0: "OP_GREATERTHAN", 0xa1: "OP_LESSTHANOREQUAL", 0xa2: "OP_GREATERTHANOREQUAL", 0xa3: "OP_MIN",
           0xa4: "OP_MAX", 0xa5: "OP_WITHIN", 0xa6: "OP_RIPEMD160", 0xa7: "OP_SHA1", 0xa8: "OP_SHA256",
           0xa9: "OP_HASH160", 0xaa: "OP_HASH256", 0xab: "OP_CODESEPARATOR", 0xac: "OP_CHECKSIG",
           0xad: "OP_CHECKSIGVERIFY", 0xae: "OP_CHECKMULTISIG", 0xaf: "OP_CHECKMULTISIGVERIFY",
           0xb1: "OP_CHECKLOCKTIMEVERIFY", 0xb2: "OP_CHECKSEQUENCEVERIFY", 0xff: "NOTANOP"}


def parse_script(script, is_input_script=False):
    if is_input_script:
        return {"body": unhexlify(script)}
    else:
        script_type = [op_code[int.from_bytes(unhexlify(script[:2]), byteorder="little")]]
        if script_type[0] == op_code[0x76]:
            script_type[1] = [op_code[int.from_bytes(unhexlify(script[2:4]), byteorder="little")]]
            script_body = unhexlify(script[4:-4])
            return {"type": script_type, "body": script_body}
        elif script_type[0] == op_code[0x6a]:
            script_body = unhexlify(script[2:])
            return {"type": script_type, "body": script_body}


class Transaction:
    def __init__(self, tx_id, json=None):
        self.tx = tx_id
        # These variables are declared None but are set in get_transaction and parse_transaction
        self.time = None
        self.raw_json = json
        self.double_spend = None
        self.raw_output = ""
        self.file_length = None
        self.file_crc = None
        self.file_crc_matches = False
        self.file = ""
        self.receiving_addresses = []
        self.sending_addresses = []
        self.get_transaction()
        self.parse_transaction()
        self.parse_file()

    def get_transaction(self):
        if json is None:
            with urllib.request.urlopen(transaction_url.format(self.tx)) as tx_page:
                if tx_page.code == 200:
                    tx_page_str = tx_page.read().decode('utf-8')
                    self.raw_json = json.loads(tx_page_str)
                else:
                    pass

    def parse_transaction(self):
        # here do actual parsing
        self.time = self.raw_json["time"]
        self.double_spend = self.raw_json["double_spend"]
        for sender in self.raw_json["inputs"]:
            self.sending_addresses.append({
                "address": sender["prev_out"]["addr"],
                "value": sender["prev_out"]["value"],
                "scripts": parse_script(sender["prev_out"]["script"], True)
            })
        for i in range(0, self.raw_json["vout_sz"]):
            receiver = self.raw_json["out"][i]
            if receiver["spent"]:
                self.receiving_addresses.append({
                    "address": receiver["addr"],
                    "value": receiver["value"],
                    "script": parse_script(receiver["script"])
                })

    def parse_file(self):
        pass
