from __future__ import print_function
import sys
import pycurl
import struct
from binascii import unhexlify, crc32
import urllib2


def get_op_return(transaction):
    try:
        transaction_sanitized = transaction[:-1]
        data = urllib2.urlopen("https://blockchain.info/tx/" + transaction_sanitized + "?show_adv=true")
        data_out = b''
        at_output = False
        for line in data:
            if 'Output Scripts' in line:
                at_output = True
            if '</table>' in line:
                at_output = False
            if at_output:
                if len(line) > 100:
                    chunks = line.split(' ')
                    this_chunk = False
                    for c in chunks:
                        if "OP_RETURN" in c:
                            this_chunk = True
                        if 'O' not in c and '\n' not in c and '>' not in c and '<' not in c and this_chunk:
                            data_out += unhexlify(c.encode('utf8'))
        length = struct.unpack('<L', data_out[0:4])[0]
        checksum = struct.unpack('<L', data_out[4:8])[0]
        data_out = data_out[8:8 + length]
        return data_out
    except Exception:
        return None


def loop_txs(TX_list, write_file=True):
    if write_file:
        with open('OP_RETURN.dat', 'wb') as return_file:
            for i in range(0, len(TX_list)):
                current_tx = get_op_return(TX_list[i])
                if current_tx is not None:
                    return_file.write(current_tx)
        return None
    else:
        total_data = b''
        for i in range(0, len(TX_list)):
            current_tx = get_op_return(TX_list[i])
            if current_tx is not None:
                total_data += current_tx
        return total_data


def parse_tx_list(file_path):
    TXs = None
    with open(file_path, 'r+') as tx_list:
        TXs = tx_list.readlines()
        if '--reverse-tx' in sys.argv:
            TXs = TXs[::-1]
    return TXs

if __name__ == '__main__':
    if len(sys.argv) >= 2:
        TXs = parse_tx_list(sys.argv[1])
        concat = loop_txs(TXs, write_file=True if '--save-op-return' in sys.argv else False)
        if '--save-op-return' not in sys.argv:
            print(concat)
    else:
        print("Usage: python tx_list [--save-op-return] [--reverse-tx]")
        print("IF: --save-op-return => OP_RETURN.dat will have your data")
