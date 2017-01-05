#    This Python script can be used to download data from the Bitcoin blockchain. 
#    Bitcoin blockchain information is downloaded from www.blockchain.info, you don't need to have btc installed on your PC.
#    original script jean.py can corrupt some binary data, this version saves data directly to a binary file without corruption.
#    
#    Python programming language must be installed in order to run this script. I am using 2.7, but 3.5 is probably ok too https://www.python.org/downloads/
#    Additionally, if you will miss some python libraries, navigate to C:/Python/Scripts/, and run command "pip install missing_library", example:
#       pip install pycurl
#    
#    Save script from BM: use right mouse click on this message in message list in Bitmessage, and select "save message as". Don't copypaste this text from Bitmessage, it will break some links.
#    
#    usage
#    python jean_b.py tx outputfilename
#    
#    examples
#    python jean_b.py 6c53cd987119ef797d5adccd76241247988a0a5ef783572a9972e7371c5fb0cc script.py
#    python jean_b.py 54e48e5f5c656b26c3bca14a8c95aa583d07ebe84dde3b7dd4a78f4e4186e713 bitcoin.pdf
#    python jean_b.py 691dd277dc0e90a462a3d652a1171686de49cf19067cd33c7df0392833fb986a cablegate-201012041811-transaction_list.txt
#    python jean_b.py 08654f9dc9d673b3527b48ad06ab1b199ad47b61fd54033af30c2ee975c588bd ami-firmware-source-code-private-key-leaked.txt
#    
#    * cablegate-201012041811-transaction_list.txt will contain a list of transaction numbers, which all can be downloaded with the same script, and then created files must be concatenated together to form a single cablegate-201012041811.7z archive
#    
import sys
import pycurl
import struct
from binascii import unhexlify, crc32
import urllib2

transaction = str(sys.argv[1])
data = urllib2.urlopen("https://blockchain.info/tx/"+transaction+"?show_adv=true&quot;")

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

file = open(str(sys.argv[2]), "wb")
file.write(dataout)
file.close()
