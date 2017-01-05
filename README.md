Wikileaks Blockchain Analysis Scripts
=====================================
**https://github.com/nickmitchko/wikileaks** 

is a collection of scripts and various software to analyze the block-chain in an effort to extract hidden files and messages.
Note: I did not author most of these scripts and only posted the ones with no licence o or a permissive licence.

#### Block-chain
<img src="http://bitcoinist.com/wp-content/uploads/2015/05/Blockchain-logo.png" height="200px"/>

## Download
* [](https://github.com/nickmitchko/wikileaks/archive/master.zip)
* Other Versions (None yet)

## Contributing
```BASH
    git clone https://github.com/nickmitchko/wikileaks.git
    - Create Pull Request
```

## Note About Python Versions

Some scripts only work with python2, some only work with python3, and some work with both. Included in the instructions are the *major* version(s) of python that each script is runnable with.

## Usage

#### jean.py - Python 2.x
###### What it does
* Downloads the data included in a block-chain transaction. Sometimes this is just empty and sometimes this may be embedded text or a file.
* Outputs the data to the console
###### Usage
```BASH
    # Syntax
    ?> python2 jean.py transaction
    # Example
    ?> python2 jean.py 6c53cd987119ef797d5adccd76241247988a0a5ef783572a9972e7371c5fb0cc
        #!/usr/bin/python3
        #
        # File downloader
        # Requires git://github.com/jgarzik/python-bitcoinrpc.git
        #
        # (c) 2013 Satoshi Nakamoto All Rights Reserved
        #
        # UNAUTHORIZED DUPLICATION AND/OR USAGE OF THIS PROGRAM IS PROHIBITED BY US AND INTERNATIONAL COPYRIGHT LAW
        
        import sys
        import os
        import jsonrpc
        import struct
        from binascii import crc32,hexlify,unhexlify
        
        
        if len(sys.argv) != 2:
            print("""\
        Usage: %s <txhash>
        
        Set BTCRPCURL=http://user:pass@localhost:portnum""" % sys.argv[0], file=sys.stderr)
            sys.exit()
        
        proxy = jsonrpc.ServiceProxy(os.environ['BTCRPCURL'])
        
        txid = sys.argv[1]
        
        tx = proxy.getrawtransaction(txid,1)
        
        data = b''
        for txout in tx['vout'][0:-2]:
            for op in txout['scriptPubKey']['asm'].split(' '):
                if not op.startswith('OP_') and len(op) >= 40:
                    data += unhexlify(op.encode('utf8'))
        
        length = struct.unpack('<L', data[0:4])[0]
        checksum = struct.unpack('<L', data[4:8])[0]
        data = data[8:8+length]
        
        if checksum != crc32(data):
            print('Checksum mismatch; expected %d but calculated %d' % (checksum, crc32(data)),
                  file=sys.stderr)
            sys.exit()
        
        sys.stdout.buffer.write(data)
    ?>
```

#### jean3.py - Python 2.x
###### What it does
* Identifies *some* file headers in the transactions from a blockchain address
* Creates a file with a list of transactions and the file headers named: addr +'_tx_list.txt'
###### Usage
```BASH
    # Syntax
    ?> python2 jean3.py addr
    # Example
    ?> python2 jean3.py 1HB5XMLmzFVj8ALj6mfBsbifRoD4miY36v
    # "All decoded data will be in the dataout folder. 1HB5XMLmzFVj8ALj6mfBsbifRoD4miY36v_file_tx_list.txt will have a list of the transactions and the type of file found in them."
```

#### Everything Else:

* Read through ./Webpage/Onion.html