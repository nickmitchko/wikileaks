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

# This is the file header of all blocks in the block chain
magic_number = bytearray('0xD9B4BEF9', encoding='ascii')
verbosity = False


def help():
    print("Usage:")


def printverbose(verbose, nonverbose):
    print(verbose) if verbosity else print(nonverbose)


def readfile(filepath):
    print("Reading Blockchain File : " + filepath)
    i = 1
    with open(filepath, "rb") as f:
        printverbose("Parsing Block " + str(i), '.')
        byte_header = f.read(4)
        if byte_header == magic_number:
            printverbose('Check Passed: File Header Matches', '.')
        else:
            printverbose('*****Check Failed*****\r\n => Exiting', '*****Check Failed*****\r\n => Exiting')


# Entry point of the script
if __name__ == '__main__':
    if len(sys.argv) <= 1:
        help()
    else:
        if len(sys.argv) == 3:
            if sys.argv[2] == 'T':
                verbosity = True
        readfile(sys.argv[1])
