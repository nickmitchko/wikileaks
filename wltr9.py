#!/usr/bin/python3

#Authorship hash: fb53f624dc3056930c5a43674a51b22095490298146a0d1a49fbe16c325ef2ca

import json
import urllib.request
from bitcoinrpc.authproxy import AuthServiceProxy
import binascii
import subprocess
import datetime
import sys
import struct
import time

DEFAULT_DIR = "dataout"

rpc_user = "USER"
rpc_pass = "PASS"

url = "http://%s:%s@127.0.0.1:8332" % (rpc_user, rpc_pass)
rpc = AuthServiceProxy(url)

def create_dir(directory: str):
	try:
		subprocess.check_output("mkdir " + directory, shell = True)
	except:
		pass

	txfile = open(directory + "/txfile", "w")
	resfile = open(directory + "/resfile", "w")
	asciifile = open(directory + "/asciifile", "w")
	return txfile, resfile, asciifile

def get_tx_from_addr(address):
	error = True
	
	print("Fetching transactions from", address)
	while error:
		try:
			dat = urllib.request.urlopen("https://blockchain.info/rawaddr/" + address)
			error = False
		except:
			print("Problem getting address", address)
			time.sleep(4)
	n_tx = json.loads( dat.read().decode() )["n_tx"]


	txlist = []
	offset = 0
	while True:
		try:
			dat = urllib.request.urlopen("https://blockchain.info/rawaddr/" + address + "?format=json&limit=50&offset="+ str(offset) )
			txs = json.loads( dat.read().decode() )["txs"]
			dat.close()
			for tx in txs:
				txlist.append(tx["hash"])
			offset += 50
		except:
			pass

		if len(txlist) == n_tx:
			break
		print(len(txlist), "/", n_tx)
		
	return txlist

def ascii_decode_attempt(word, txid, txtime, ascii_file):
	try:
		asciitext = word.decode()
		print("txid: " + txid)
		print(asciitext)
		ascii_file.write("txid: " + txid + "\n")
		ascii_file.write("Translation attempt: " + asciitext + "\n")
		ascii_file.write("Timestamp: " + txtime + "\n\n")
	except UnicodeError:
		return

def get_file_tx(txid: str, txfile, resfile, asciifile, directory, keepwritten=False):
	data = b''
	tx = rpc.getrawtransaction(txid,1)
	txtime = datetime.date.fromtimestamp(tx["time"]).ctime()
	for txout in tx['vout']:
		pubkeytype = txout['scriptPubKey']['type']
		for op in txout['scriptPubKey']['asm'].split(' '):
			if not op.startswith('OP_') and len(op) > 1:
				try:
					word = binascii.unhexlify(op.encode('utf8'))
					data += word
					ascii_decode_attempt(word, txid, txtime, asciifile)
				except:
					pass
	try:
		length = struct.unpack('<L', data[0:4])[0]
		checksum = struct.unpack('<L', data[4:8])[0]
		dat = data[8:8+length]
		if checksum != binascii.crc32(dat):
			#print('Checksum mismatch; expected %d but calculated %d' % (checksum, crc32(data)),
			raise Exception
		print("Checksum found at tx " + txid)
		resfile.write("Checksum found at tx " + txid)
		data = dat
	except:
		pass
		
	filename = directory + ".txt"
	fd = open(filename,"wb")
	fd.write(data)
	fd.close()

	output = subprocess.check_output("triddir/trid " + filename, shell=True).decode()
	if "Unknown!" not in output:
		trid = output.split('\n')[6]
		out_str = txid + "\n" \
					+ trid + "\n" \
					+ txtime + "\n" \
					+ "from block " + str(rpc.getblock(tx['blockhash'])['height']) + "\n" \
					+ str(tx['time']) + "\n" \
					+ str(tx['blocktime']) + "\n" \
					+ "\n"
		print(out_str)
		resfile.write(out_str)
		txfile.write(txid + "\n")

		if keepwritten:
			f = open(directory + "/" + txid, 'wb')
			f.write(data)
			f.close()

	subprocess.check_output("rm " + filename, shell=True)

def get_file_addr(addr, directory: str):
	txlist = get_tx_from_addr(addr)
	txfile, resfile, asciifile = create_dir(directory)
	for txid in txlist:
		get_file_tx(txid, txfile, resfile, asciifile, directory)

def get_file_block(blockh, directory: str):
	txfile, resfile, asciifile = create_dir(directory)
	block = rpc.getblock( rpc.getblockhash(blockh) )
	for txid in block["tx"]:
		get_file_tx(txid, txfile, resfile, asciifile, directory)

def get_file_block_range(bh1: int, bh2: int, directory: str):
	txfile, resfile, asciifile = create_dir(directory)
	for i in range(bh1, bh2+1):
		block = rpc.getblock( rpc.getblockhash(i) )
		for txid in block["tx"]:
			get_file_tx(txid, txfile, resfile, asciifile, directory)

