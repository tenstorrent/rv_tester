# Checks that plusargs are not duplicately defined
# Assumes there's no declares in C/C++ files

# For each header file, if a line begins with DECLARE_, add it to a dict
# If it is already in that dict, then add it to error array
# At end of execution, if error array is not empty, return 1

import os
import sys


IGNORE_DIRECTORIES = ["./.git", "./infra", "./build", "./regressions"]
DECLARE_TYPES = ["DECLARE_bool", "DECLARE_int32", "DECLARE_int64", "DECLARE_uint64", "DECLARE_double", "DECLARE_string"]


# recursive
# returns list of all header files
def scan_directory(directory, header_files):
    for file in os.scandir(directory):
        filename = os.fsdecode(file)
        if filename not in IGNORE_DIRECTORIES:
            
            # if directory, go in it and scan
            if os.path.isdir(filename):
                scan_directory(filename, header_files)

            # if file, open and look for header files
            if filename.endswith(".h") or filename.endswith(".hpp"):
                header_files.append(filename)


# Looks for duplicate declares
# returns 0 if no duplicates, 1 if there is at least one duplicate
def check_duplicate_declares(header_files):
    result = 0
    all_declares = {}
    for h in header_files:
        with open(h, "r") as f:
            for line in f:
                for dec_type in DECLARE_TYPES:
                    if line.startswith(dec_type):
                        dec = line.split(";")[0]
                        # add to dict with filename
                        if dec not in all_declares:
                            all_declares[dec] = h
                        else:
                            print(f"{dec} declared in {h} and {all_declares[dec]}")
                            result = 1;

    return result

def main():
    directory = os.fsencode(".")
    header_files = []

    scan_directory(directory, header_files)
    sys.exit(check_duplicate_declares(header_files))


if __name__ == "__main__":
    main()