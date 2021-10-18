#!/usr/bin/python
import os
import fnmatch

def checkAndFixFileHeaderComment(fileName, template):
    lines = []
    with open(fileName, 'rw') as f:
        lines = f.readlines()
    
    print(template)
    for line in lines:
        print(line)



def main():
    template = "/*\n"

    with open('tools/copyright.txt', 'r') as reader:
        line = reader.readline()
        while line != '':
           template += " * " + line
           line = reader.readline()
    template += " */"

    for root, dir, files in os.walk(os.curdir):
        if ".git/" in root:
            continue

        for item in fnmatch.filter(files, "*.h"):
            checkAndFixFileHeaderComment(root + os.sep + item, template)

        for item in fnmatch.filter(files, "*.cpp"):
            checkAndFixFileHeaderComment(root + os.sep + item, template)


if __name__ == "__main__":
    main()