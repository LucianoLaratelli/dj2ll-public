#!/usr/bin/env python3

import os
import sys
import pprint
from pygments import highlight
from pygments.lexers import JavaLexer
from pygments.formatters import Terminal256Formatter as T256F

ignore_list = ["good30.dj"]

undotted_method_call = ["good07.dj", "good15.dj", "good17.dj", "goodMine1.dj"]

vtable = ["good08.dj", "good09.dj", "good19.dj", "good33.dj"]

null = ["good10.dj", "good27.dj", "good24.dj", "good29.dj", "good31.dj"]


def main():
    if len(sys.argv) == 1:
        # test all programs except those we know don't work yet
        files = sorted(
            [f for f in os.listdir("test_programs/good") if f not in ignore_list]
        )
    elif sys.argv[1] == "um":
        files = undotted_method_call
    elif sys.argv[1] == "vt":
        files = vtable
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
