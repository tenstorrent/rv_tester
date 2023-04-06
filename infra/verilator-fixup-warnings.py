#! /usr/bin/env python3

import re

class FixupFile:

    def __init__(self, filename):

        self.filename = filename
        self.warnings = {}

    def add_warning(self, code, line):

        if line not in self.warnings:
            self.warnings[line] = []
        if code not in self.warnings[line]:
            self.warnings[line].append(code)

    def fixup(self):

        new_file = []
        with open(self.filename) as f:
            lineno = 0
            for line in f:
                lineno += 1

                if lineno in self.warnings:
                    for warning in self.warnings[lineno]:
                        new_file.append("/* verilator lint_off " + warning + "*/ // FIXME inserted automatically by verilator-fixup-warnings.py\n")

                new_file.append(line)

                if lineno in self.warnings:
                    for warning in self.warnings[lineno]:
                        new_file.append("/* verilator lint_on " + warning + "*/\n")


        with open(self.filename, 'w') as f:
            f.write("".join(new_file))


class VerilatorWarnings:

    def __init__(self):
        self.regex = re.compile("%Warning-(?P<code>\S+): (?P<file>[^:]+):(?P<line>\d+):")

    def fixup(self, io):

        files = {}

        for line in io:

            match = re.search(self.regex, line)
            if match:

                filename = match.group("file")
                if filename not in files:
                    files[filename] = FixupFile(filename)
                f = files[filename]
                f.add_warning(match.group("code"), int(match.group("line")))


        for f in files.values():
            try:
                f.fixup()
            except PermissionError:
                print("could not fixup " + f.filename)

if __name__ == "__main__":
    import fileinput
    VerilatorWarnings().fixup(fileinput.input())
