#!/usr/bin/env python3

import os
import sys
import pprint
from pygments import highlight
from pygments.lexers import JavaLexer
from pygments.formatters import Terminal256Formatter as T256F

ignore_list = [
    "good6.dj",
    "good7.dj",
    "good8.dj",
    "good15.dj",
    "good17.dj",
    "good19.dj",
    "good20.dj",
    "good21.dj",
    "good30.dj",
    "good33.dj",
]


def main():
    pp = pprint.PrettyPrinter(indent=4)
    files = os.listdir("test_programs/good")
    files = [file for file in files if file not in ignore_list]
    files.sort()
    for file in files:
        fileName = f"test_programs/good/{file}"
        with open(fileName) as f:
            infoStr = f"File = {file}"
            asterisks = len(infoStr) * "*"
            print(f"{asterisks}\n{infoStr}\n{asterisks}")
            print(highlight(f.read(), JavaLexer(), T256F(style="monokai")))
            os.system(f"./dj2ll {fileName}")
            print(asterisks)
            os.system(f"./{file[0:-3]}")
            print(asterisks)
            reply = str(input("(press [enter] to continue):")).strip()
            if reply != "":
                sys.exit()


if __name__ == "__main__":
    main()
