#!/usr/bin/env python3
# Copyright (c) 2024 Jakub Zimnol
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

from argparse import ArgumentParser
from hashlib import sha256
import os


def _main():
    parser = ArgumentParser(
        description='Append SHA256 hash to the end of the firmware file thath will be sent to the device.')
    parser.add_argument('-t', '--target-file', help='Path to the firmware file', required=True)

    args = parser.parse_args()

    # check if given file exist
    if not os.path.exists(args.target_file):
        print(f"File {args.target_file} does not exist")
        raise FileNotFoundError

    # ensure that given file is a *.bin file
    file_path_no_ext = args.target_file.rsplit('.', 1)[0]
    binary_file_path = file_path_no_ext + '.bin'
    output_file_path = file_path_no_ext + "_fota_image" + '.bin'
    print(f"Using binary: {binary_file_path}")

    try:
        os.remove(output_file_path)
    except FileNotFoundError:
        pass

    with open(binary_file_path, 'rb') as file:
        binary_file_data = file.read()

    binary_sha256 = sha256(binary_file_data)

    with open(output_file_path, 'wb') as file:
        file.write(binary_file_data)
        padding = b'\x00' * (256 - 32)
        file.write(padding)
        file.write(binary_sha256.digest())

    print(f"Output path: {output_file_path}")


if __name__ == '__main__':
    _main()
