#!/usr/bin/env python3

# A script for repacking data from a compressed .zip to an uncompressed .psp that's easier to read
# back from the disk.
#
# Usage: repack_data.py <path to data.zip> <output path to data.psp>

import argparse
import os
import shutil
import struct
import zipfile

# Parse command line arguments.

parser = argparse.ArgumentParser(description = "Repack VVVVVV data.zip to data.psp")

parser.add_argument("input_file", type = str, help = "Path to data.zip")
parser.add_argument("output_file", type = str, help = "Output path where data.psp should be stored")

wd = "repack_data_wd"
os.mkdir(wd)

args = parser.parse_args()
input_filename = args.input_file
output_filename = args.output_file

# Extract data.zip.

print("Extracting data.zip")
data_dir = os.path.join(wd, "data")
with zipfile.ZipFile(input_filename, "r") as z:
   os.mkdir(data_dir)
   z.extractall(data_dir)

# Walk through all the entries and construct a list from them.

print("Creating entry table")
entries = []
for path, _, files in os.walk(data_dir):
   for file in files:
      file_path = os.path.relpath(os.path.join(path, file), start = data_dir)
      entries.append(file_path)

# Build the data.psp file.

print("Creating data.psp")
with open(output_filename, "wb") as f:
   # Magic header.
   f.write(b"V4PSP")

   # Entry count. Max entries is 255, the stock data.zip only uses 69 (nice) at the moment.
   print(" - total entries:", len(entries))
   n_entries = struct.pack("<Bxxx", len(entries))
   f.write(n_entries)

   # Entry list.
   print(" - writing entry list")
   for path in entries:
      path_in_data = os.path.join(data_dir, path)
      filesize = os.path.getsize(path_in_data)
      entry = struct.pack("<I", len(path)) + bytes(path, "UTF-8") + struct.pack("<I", filesize)
      f.write(entry)

   # Files.
   # The offsets of files can be determined easily by accumulating the file sizes from the file list.
   print(" - writing files")
   for path in entries:
      print("   -", path)
      path_in_data = os.path.join(data_dir, path)
      with open(path_in_data, "rb") as ff:
         content = ff.read()
         f.write(content)

# Clean up work directory.
print("Cleaning up")
shutil.rmtree(wd)
