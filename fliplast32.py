import sys
import os


def flip_last32(file_path):
    file_size = os.path.getsize(file_path)
    with open(file_path, 'rb+') as in_file:
        try:
            if file_size > 32:
                in_file.seek(file_size-32)
                last_32 = in_file.read(32)[::-1]    # reverse the last 32
                in_file.seek(file_size-32)
                in_file.write(last_32)
        except Exception:
            pass


if __name__ == '__main__':
    if len(sys.argv) == 2:
        flip_last32(sys.argv[1])
