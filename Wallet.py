import urllib.request
import json
from binascii import unhexlify, crc32
from Transaction import Transaction

wallet_url = "https://blockchain.info/rawaddr/{0}?offset={1}"


class Wallet:
    def __init__(self, wallet_address):
        self.address = wallet_address
        self.transactions, self.transaction_number = self.parse_wallet()

    def parse_wallet(self):
        transactions = None
        transaction_number = 0
        transaction_offset = 0
        serialized_transactions = []
        with urllib.request.urlopen(wallet_url.format(self.address, transaction_offset)) as wallet_page:
            if wallet_page.code == 200:
                raw_json = json.loads(wallet_page.read().decode('utf-8'))
                transactions = raw_json["txs"]
                transaction_number = raw_json["n_tx"]
            else:
                print("Your API limit may be reached\tcode: " + str(wallet_page.code))
        while transaction_offset + 50 < transaction_number:
            transaction_offset += 50
            with urllib.request.urlopen(wallet_url.format(self.address, transaction_offset)) as wallet_page:
                if wallet_page.code == 200:
                    raw_json = json.loads(wallet_page.read().decode('utf-8'))
                    transactions.append(raw_json["txs"])
                else:
                    print("Your API limit may be reached\tcode: " + str(wallet_page.code))
        for transaction_i in transactions:
            serialized_transactions.append(Transaction(transaction_i))
        return serialized_transactions, transaction_number

