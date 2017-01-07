import sys
import os


def concat_files(file_path):
    files = ''
    with open(file_path, 'r') as in_file:
        try:
            files = in_file.readlines()
        except Exception:
            pass
    i = 0
    directory = file_path[:file_path.rfind('/') + 1]
    with open(file_path + 'concat', 'wb') as out_file:
        while True:
            try:
                with open(directory + files[i][:-1], 'rb') as tx_file:
                    print("Adding: " + files[i][:-1])
                    file_size = os.path.getsize(directory + files[i][:-1])
                    try:
                        bytes = tx_file.read(file_size)
                        out_file.write(bytes)
                    except EOFError:
                        pass
                i += 1
            except Exception:
                break

if __name__ == '__main__':
    if len(sys.argv) == 2:
        concat_files(sys.argv[1])
