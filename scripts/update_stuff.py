#!/usr/bin/python2.7

import os
import sys
import subprocess
import fileinput
import time

# the version of the release
with open("version.txt") as f: version = f.read()

def getVersionTuple(v):
    return tuple(map(int, (v.split("."))))

version_major = str(getVersionTuple(version)[0])
version_minor = str(getVersionTuple(version)[1])
version_patch = str(getVersionTuple(version)[2])

# update version in the header file
print("updating the version in the header file")
doctest_contents = ""
for line in fileinput.input(["../doctest/parts/doctest_fwd.h"]):
    if line.startswith("#define DOCTEST_VERSION_MAJOR "):
        doctest_contents += "#define DOCTEST_VERSION_MAJOR " + version_major + "\n"
    elif line.startswith("#define DOCTEST_VERSION_MINOR "):
        doctest_contents += "#define DOCTEST_VERSION_MINOR " + version_minor + "\n"
    elif line.startswith("#define DOCTEST_VERSION_PATCH "):
        doctest_contents += "#define DOCTEST_VERSION_PATCH " + version_patch + "\n"
    elif line.startswith("#define DOCTEST_VERSION_STR "):
        doctest_contents += "#define DOCTEST_VERSION_STR \"" + version + "\"\n"
    else:
        doctest_contents += line

readme = open("../doctest/parts/doctest_fwd.h", "w")
readme.write(doctest_contents)
readme.close()

# run generate_html.py
print("generating html documentation from markdown")
os.system("python generate_html.py")

examples = "../examples/"
'''
# create readme files in examples with 'try it online' badges with permalinks
examples_skip = ["multiprocess"]

for dir in os.listdir(examples):
    if not os.path.isdir(examples + dir):
        continue
    
    # skip folders with more than 1 source file
    sources = [s for s in os.listdir(examples + dir) if ".cpp" in s]
    if dir in examples_skip or len(sources) > 1 or len(sources) == 0:
        continue
    
    print("generating readme for example '" + examples + dir + "'")
    
    proc = subprocess.Popen('python send_to_wandbox.py ../doctest/ ' + examples + dir + "/" + sources[0], stdout = subprocess.PIPE)
    url = proc.stdout.read().strip()
    
    if not url.startswith("http"):
        print("skipping current example because of crappy url: '" + url + "'\n")
        continue
    
    readme = open(examples + dir + "/README.md", "w")
    readme.write("[![Try it online](https://img.shields.io/badge/try%20it-online-orange.svg)](" + url + ")")
    readme.close()
'''

# update main readme 'try it online' badge permalink
print("updating main readme")
proc = subprocess.Popen('python send_to_wandbox.py ../doctest/ ' + examples + "hello_world/main.cpp", stdout = subprocess.PIPE)
url = proc.stdout.read().strip()

readme_contents = ""
for line in fileinput.input(["../README.md"]):
    if line.startswith("[![Try it online]"):
        readme_contents += "[![Try it online](https://img.shields.io/badge/try%20it-online-orange.svg)](" + url + ")\n"
    else:
        readme_contents += line

readme = open("../README.md", "w")
readme.write(readme_contents)
readme.close()
