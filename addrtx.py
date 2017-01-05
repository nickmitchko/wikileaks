#!/usr/bin/python3

import sys
from wltr8 import get_file_addr

#Get all transactions from address argv1, store file info in argv2
get_file_addr(sys.argv[1], sys.argv[2])
