########################
#   blockextractor.py
#
#   A Script to extract individual blocks from the block chain
#
#   Usage: python blockextractor.py /path/to/bootstrap.dat
#
#   Output: a bunch blockchain blocks in the same folder as bootstrap.dat
#
#
#   Information on the blockchain: https://en.bitcoin.it/wiki/Block
########################

import sys
import os

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

# This is the file header of all blocks in the block chain
magic_number = bytearray(b'\xf9\xbe\xb4\xd9')
verbosity = False
dryrun = False

def help():
    print("------------------------------- Usage -------------------------------")
    print("")
    print("python3 blockextractor.py [PATH TO BLOCK-CHAIN] [OPTIONS]")
    print("")
    print("python3 blockextractor.py [PATH TO BLOCK-CHAIN] [--verbose] [--trial]")
    print("")
    print("------------------------------ Options ------------------------------")
    print("")
    print("--verbose")
    print("                     Writes filenames and block info to console")
    print("")
    print("--trial")
    print("                     Parses the Block-Chain file but does not write")
    print("                     the output to a file; good for testing a file")
    print("")


def printverbose(verbose, nonverbose):
    if verbosity:
        if verbose != '':
            print(verbose)
    else:
        if nonverbose != '':
            print(nonverbose)


def readfile(filepath):
    directory = filepath[:filepath.rfind('/') + 1]
    print( "Reading Blockchain File : "+ bcolors.UNDERLINE + filepath + bcolors.ENDC)
    file_size = os.path.getsize(filepath)
    if dryrun:
        file_size = 10000
    total_bytes = 0
    i = 0
    with open(filepath, "rb") as f:
        while total_bytes < file_size:
            try:
                printverbose("Parsing Block " + str(i), '')
                byte_header = f.read(4)
                if byte_header == magic_number:
                    printverbose(bcolors.OKGREEN + 'Pass' +bcolors.ENDC + ': File Header Matches', '')
                    block_length = f.read(4)
                    b_l_bytes = int.from_bytes(block_length, byteorder='little')
                    printverbose(bcolors.OKGREEN + 'Pass' +bcolors.ENDC + ': Block Length is ' + str(b_l_bytes), '')
                    block_content = f.read(b_l_bytes)
                    if dryrun is False:
                        save_block(block_length, block_content, i + 1, directory)
                    printverbose(bcolors.OKGREEN + 'Pass' +bcolors.ENDC + ': Block Parsed', '')
                    i += 1
                    total_bytes += 4 + 4 + b_l_bytes
                    printverbose('',bcolors.OKBLUE + str(total_bytes/file_size) + ' % ' + bcolors.ENDC)
                else:
                    printverbose('*** '+ bcolors.FAIL + 'Check Failed'+ bcolors.ENDC +'  ***\r\n => Exiting', '*** Check Failed  ***\r\n => Exiting')
            except EOFError:
                pass


def save_block(blk_len, blk_content, number, directory):
    filename = directory + "blk" + str(number) + '.dat'
    printverbose(filename, '')
    with open(filename, 'wb') as file:
        file.write(magic_number)
        file.write(blk_len)
        file.write(blk_content)


# Entry point of the script
if __name__ == '__main__':
    if len(sys.argv) <= 1:
        help()
    else:
        if len(sys.argv) >= 3:
            if sys.argv[2] == '--verbose':
                verbosity = True
            if sys.argv[2] == '--trial':
                dryrun = True
        if len(sys.argv) >= 4:
            if sys.argv[3] == '--trial':
                dryrun = True
            if sys.argv[3] == '--verbose':
                verbosity = True
        readfile(sys.argv[1])
