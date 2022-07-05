#!/usr/bin/python3
import os
import re


def get_all_units() -> list():
    l = []
    cur_name = os.getcwd()
    for root, _, files in os.walk("./tests/", topdown=False):
        for f in files:
            if(re.search("^.*\.o$", f)):
                fullpath = cur_name + root[1:] + "/" + f
                l.append(fullpath)
    return l


def exec_test(files: str) -> bool:
    for f in files:
        print("Test " + f)
        if(os.system(f)):   # Return non-zero if failed
            print("\n\nError on: \x1b[1;31m{}\x1b[0m\n\n".format(f))
            return False
    return True


if __name__ == '__main__':
    files = get_all_units()
    if not exec_test(files):
        exit(1)
