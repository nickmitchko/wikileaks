#!/usr/bin/python3

import sys
from wltr8 import get_file_block

#arg1 = block # (height)
#arg2 = directory name to store files in
get_file_block( int(sys.argv[1]), sys.argv[2] )
