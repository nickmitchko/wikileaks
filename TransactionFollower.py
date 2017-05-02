import sys
import re
from Transaction import Transaction, transaction_length


class TerminalColors:
    HEADER = '\033[95m'
    OK_BLUE = '\033[94m'
    OK_GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    END_C = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


def follow_block_chain_transaction(tx):
    transaction = Transaction(tx_id=tx)
    print(transaction.file)
    # After this crawl the addresses with spent $$


def parse_arguments():
    if len(sys.argv) >= 2:
        tx_match = re.search("[a-f0-9]{64}", sys.argv[1])
        tx_start = tx_match.group()
        if len(tx_start) == transaction_length:
            follow_block_chain_transaction(tx_start)
    else:
        print(TerminalColors.FAIL + "Not Enough Arguments" + TerminalColors.END_C)


if __name__ == '__main__':
    parse_arguments()
