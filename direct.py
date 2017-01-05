########################
#   direct.py
#
#   A Script to apply 'directing' a binary file
#
#   'directing':
#       Turns a left-to-right read file into a up-to-down read file
#       splicing at the size of the block. This cryptographically
#       insecure read direction changer helps to guard against known
#       plain-text attacks if applied before and after encryption.
#
#       Example:
#           normal   => e pluribus unum : 65 20 70 6C 75 72 69 62 75 73 20 75 6E 75 6D (0A)
#           direct(4) => 65 20           : 67 76 72 66 50 59 50 ED 26 76 77 70 0C 22 35 5A
#                       70 6C
#                       75 72
#                       69 62
#                       75 73
#                       20 75
#                       6E 75
#                       6D 0A
#
#   Usage:
#       python  direct.py [direct Direction] file [Block Length]
#       python3 direct.py [direct Direction] file [Block Length]
#
#   direct Direction:
#       --direct:    Outputs 'directed' hex representation of file to console
#       --undirect:  Outputs 'un-directed' file (in ascii) to console
#
#   Block Length:
#       Number >= 4: Length of direct
#       Note: this number must be smaller than the length of the text or file
#
#   Information on the block-chain:
#       https://en.bitcoin.it/wiki/
########################

import sys
import os


def help_():
    print("   direct.py")
    print("")
    print("   A Script to apply 'directing' a binary file")
    print("")
    print("   'directing':")
    print("       Turns a left-to-right read file into a up-to-down read file")
    print("       splicing at the size of the block. This cryptographically")
    print("       insecure read direction changer helps to guard against known")
    print("       plain-text attacks if applied before and after encryption.")
    print("")
    print("       Example:")
    print("           normal   => e pluribus unum : 6520706C75726962757320756E756D (0A)")
    print("           direct(4) => gvrfPYP...")
    print("                       65 20           : 67767266505950ED267677700C22355A")
    print("                       70 6C")
    print("                       75 72")
    print("                       69 62")
    print("                       75 73")
    print("                       20 75")
    print("                       6E 75")
    print("                       6D 0A")
    print("")
    print("   Usage:")
    print("       python  direct.py [Direction Change] file [Block Length]")
    print("       python3 direct.py [Direction Change] file [Block Length]")
    print("")
    print("   Direction Change:")
    print("       --direct:    Outputs 'directed' hex representation of file to console")
    print("       --un-direct:  Outputs 'un-directed' file (in ascii) to console")
    print("")
    print("   Block Length:")
    print("       Number >= 4: Length of direct")
    print("       Note: this number must be smaller than the length of the text or file")
    print("")
    print("   Information on the block-chain:")
    print("       https://en.bitcoin.it/wiki/")


def direct(file_path, block_sz):
    directory = file_path[:file_path.rfind('/') + 1]
    file_size = os.path.getsize(file_path)
    total_bytes = 0
    with open(file_path, "rb") as infile:
        try:
            f = infile.readlines()

        except EOFError:
            pass


def un_direct(file_path, block_sz):
    with open(file_path, "rb") as infile:
        hex_string = ''
        try:
            f = infile.readlines()
            c = f[0]
            off = 0
            lf = ''
            while lf != '\n':
                for i in range(0, block_sz):
                    hex_string += chr(c[(i * block_sz) + off])
                    lf = chr(c[i * block_sz + off + 1])
                off += 1
        except Exception:
            pass
        array = str(bytearray.fromhex(hex_string))
        ind = array.index('bytearray(b')+12
        ind2= array.index(')', ind) -1
        sys.stdout.write(array[ind:ind2])
        sys.stdout.flush()
        print()


if __name__ == '__main__':
    argv_length = len(sys.argv)
    if argv_length < 4:
        help_()
    else:
        direct_ = True
        if sys.argv[1] == '--direct':
            direct_ = True
        elif sys.argv[1] == '--un-direct':
            direct_ = False
        file = sys.argv[2]
        block_length = -1
        if argv_length == 4:
            block_length = int(sys.argv[3])
        if direct_:
            direct(file, block_length)
        else:
            un_direct(file, block_length)
else:
    help_()
