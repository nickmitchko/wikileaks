#!/usr/bin/python3
########################
#   bcrdr2_file_extract.py
#
#   A Script to extract files from the block-chain
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
import sqlite3


class DbReader:

    def __init__(self, database_path="./out.db"):
        self.database = sqlite3.connect(database_path)
        self.database_connection = self.database.cursor()

    def get_entry_by_file_header(self):
        return self.database_connection.execute("")