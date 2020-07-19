#!/usr/bin/env python3

import os
import sys
import pprint
from pygments import highlight
from pygments.lexers import JavaLexer
from pygments.formatters import Terminal256Formatter as T256F

ignore_list = [
    "good06.dj",
    "good07.dj",
    "good08.dj",
    "good09.dj",
    "good15.dj",
    "good17.dj",
    "good19.dj",
    "good20.dj",
    "good21.dj",
    "good22.dj",
    "good30.dj",
    "good33.dj",
    "goodMine1.dj",
]


def main():
    pp = pprint.PrettyPrinter(indent=4)
    files = sorted(
        [f for f in os.listdir("test_programs/good") if f not in ignore_list]
    )
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
                break
    os.system("rm good*")


if __name__ == "__main__":
    main()