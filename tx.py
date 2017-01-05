#!/usr/bin/python3

import subprocess
import sys
from wltr9 import create_dir, get_file_tx

txid = sys.argv[1]
directory = sys.argv[2]
txfile, resfile, asciifile = create_dir(directory)

get_file_tx(txid, txfile, resfile, asciifile, directory, True)


