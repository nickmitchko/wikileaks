#!/usr/bin/python3
########################
#   extract7z.py
#
#   A Script to extract 7z files from the block-chain
#
#   First:
#       Run the following command on the block-chain to generate the 7z grep stub
#       $ LANG=C grep -obUaP "\x37\x7A\xBC\xAF\x27\x1C" /path/to/blocks/blk*.dat | tee possible-7z.log
#                                                              ┌-----------------------^^^^^^^^^^^^^^^
#   Then:                                                      │
#       Run this python script on the log file generated "possible-7z.log"
#       Specify an output directory for parsed archives
#       $ python3 extract7z.py /path/to/possible7z.log ./extractedArchives/
#
#   Usage:
#       python3 extract7z.py grep_output 7z_dir
#
#   Output:
#       extract 7z files from the block-chain
#
#   Note:
#       This currently only works for SINGLE transaction 7z files. Other files will be produced
#       will will be corrupted.
#
#   TODOs by Priority:
#       TODO: Implement CRC-32 Checking for header and footer
#       TODO: Implement Footer verification
#       TODO: Implement Multi-transaction extraction
#       TODO: Add integration with bcrdr2 output database
#       TODO: Add different file types with header information
#
#   Information on 7z format:
#       http://7-zip.org/recover.html
########################

import sys
import os

magic_number = b'\x37\x7A\xBC\xAF\x27\x1C'


def read_line_(file):
    string = ""
    while True:
        char = file.read(1)
        if char == b'':
            exit()
        if char == b'\xbc':
            while char != b'\x0a' and char != b'':
                char = file.read(1)
                print(char)
            print(string)
            return string
        string += str(char)[2:3]
    return string


def process_grep_output(file_path, out_dir):
    file_no = 0
    directory = file_path[:file_path.rfind('/') + 1]
    with open(file_path, "rb") as f:
            try:
                grep_str = read_line_(f)
                while grep_str != '':
                    arr = grep_str.split(':')
                    with open(directory + arr[0], 'rb') as block:
                        header_offset = int(arr[1])
                        block.seek(header_offset)
                        if block.read(6) == magic_number:
                            version = block.read(2)
                            block.seek(header_offset+6+2+4)
                            end_header_relative_offset = int.from_bytes(block.read(8), byteorder="little")
                            end_header_length = int.from_bytes(block.read(8), byteorder="little")
                            block.seek(header_offset+0x20)
                            first_byte = block.read(1)
                            compression_lzma = True if first_byte == b'\x00' else False
                            block.seek(header_offset)
                            file_contents = block.read(32 + end_header_relative_offset + end_header_length)
                            save_archive(file_contents, file_no, out_dir)
                            print("Version: " +
                                  str(int.from_bytes(version, byteorder="little")) +
                                  "\tEncoding: " + ("LZMA" if compression_lzma else "LZMA2 or AES") +
                                  "\tEnd Header: " + str(end_header_relative_offset + end_header_length) )
                            file_no += 1
                    grep_str = read_line_(f)
            except EOFError:
                pass


def save_archive(archive_content, output_number, directory):
    filename = directory + "/archive" + str(output_number) + '.7z'
    with open(filename, 'wb') as file:
        file.write(archive_content)


# Entry point of the script
if __name__ == '__main__':
    if len(sys.argv) == 3:
        process_grep_output(sys.argv[1], sys.argv[2])
    else:
        print("Too few arguments")
