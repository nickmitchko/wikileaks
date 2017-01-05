import sys, getopt
import json
import pycurl
import struct
import cStringIO
import pprint;
import jsonrpclib
import subprocess;
import os;
from binascii import unhexlify, crc32

# Settings
rpcserver = "http://username:password@localhost:8332"

# Bitcoin RPC Client
server = jsonrpclib.Server(rpcserver)

def decode_transaction(transaction):
    "Decodes an individual transaction"

    try:
        rawTx = server.getrawtransaction(transaction)
        tx = server.decoderawtransaction(rawTx)

        data = b''
        for txout in tx['vout']:
            if "OP_RETURN" in txout['scriptPubKey']['asm']:
              for op in txout['scriptPubKey']['asm'].split(' '):
                  if not op.startswith('OP_'):
                      print(transaction+": contains OP_RETURN")
                      data = unhexlify(op.encode('utf8'))
                      file = open('opreturns.txt', "ab")
                      file.write(data+"\r\n")
                      file.close()
    
    except Exception, e:
        print(jsonrpclib.history.request)
        print(str(e))
        
    return

def get_transactions_for_block(block):
    "Gets the transactions for a given block"
    print("Checking block: "+str(block))
    hash = server.getblockhash(block)
    block = server.getblock(hash)
    for transaction in block["tx"]:
        decode_transaction(transaction)
    return

# Main entry point of application
type = sys.argv[1]

if type == 'transactions':
    transaction = sys.argv[2]
    decode_transaction(transaction)

elif type == 'blocks':
    print("Blockchain length: " + str(server.getblockcount()))
    start = int(sys.argv[2])
    end = int(sys.argv[3])
    for block in range(start, end):
        get_transactions_for_block(block)



