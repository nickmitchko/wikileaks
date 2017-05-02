#!/usr/bin/python3
########################
#   extract7z.py
#
#   A Script to extract 7z files from the block-chain
#
#   First:
#       Run the following command on the block-chain to generate the 7z grep stub
#       $ LANG=C grep -obUaP "\x37\x7A\xBC\xAF\x27\x1C" /path/to/blocks/blk*.dat | tee possible-7z.log
#                                                            
#   Then:
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
                            header_crc = block.read(4)
                            footer_offset_bytes = block.read(8)
                            footer_length_bytes = block.read(8)
                            footer_crc = block.read(4)
                            footer_relative_offset = int.from_bytes(footer_offset_bytes, byteorder="little")
                            footer_length = int.from_bytes(footer_length_bytes, byteorder="little")
                            contents = block.read(footer_relative_offset)
                            footer = block.read(footer_length)
                            first_byte = contents[0]
                            compression_lzma = True if first_byte == b'\x00' else False
                            block.seek(header_offset)
                            save_archive(version, header_crc, footer_offset_bytes, footer_length_bytes, footer_crc, contents, footer,
                                         file_no, out_dir)
                            print("Version: " +
                                  str(int.from_bytes(version, byteorder="big")) +
                                  "\tEncoding: " + ("LZMA" if compression_lzma else "LZMA2 or AES") +
                                  "\tEnd Header: " + str(footer_relative_offset + footer_length) )
                            file_no += 1
                    grep_str = read_line_(f)
            except EOFError:
                pass


def save_archive(version, header_crc, footer_relative_offset, footer_length, footer_crc, contents, footer, output_number, directory):
    filename = directory + "/archive" + str(output_number) + '.7z'
    with open(filename, 'wb') as file:
        file.write(magic_number)
        file.write(version)
        file.write(header_crc)
        file.write(footer_relative_offset)
        file.write(footer_length)
        file.write(footer_crc)
        file.write(contents)
        file.write(footer)


# Entry point of the script
if __name__ == '__main__':
    if len(sys.argv) == 3:
        process_grep_output(sys.argv[1], sys.argv[2])
    else:
        print("Too few arguments")
