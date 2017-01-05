import sys
import pycurl
import struct
from binascii import unhexlify, crc32
import urllib2

transaction = str(sys.argv[1])
data = urllib2.urlopen("https://blockchain.info/tx/"+transaction+"?show_adv=true")
dataout = b''
atoutput = False

for line in data:

        if 'Output Scripts' in line:
            atoutput = True

        if '</table>' in line:
            atoutput = False

        if atoutput:
            if len(line) > 100:
                chunks = line.split(' ')
                for c in chunks:
                    if 'O' not in c and '\n' not in c and '>' not in c and '<' not in c:
                        dataout += unhexlify(c.encode('utf8'))

length = struct.unpack('<L', dataout[0:4])[0]
checksum = struct.unpack('<L', dataout[4:8])[0]
dataout = dataout[8:8+length]

print dataout
