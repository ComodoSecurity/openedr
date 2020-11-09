#!/usr/bin/env python3

__author__ = "Peter Spiess-Knafl"
__email__ = "dev@spiessknafl.at"
__license__ = "MIT"

from changelog import ChangeLog
from subprocess import call
from shutil import copyfile
import os
import os.path
import sys
import urllib.request
import subprocess

files = []


def normalize_tag(tag):
    return ''.join(filter(lambda x: x.isdigit() or x == '.', tag))


def cleanup():
    for f in files:
        if os.path.isfile(f):
            os.remove(f)


def main():
    tag = subprocess.Popen(["git", "describe"],stdout = subprocess.PIPE).communicate()
    changelog = "CHANGELOG.md"
    user = "cinemast"
    name = os.path.basename(os.getcwd())

    cl = ChangeLog(changelog)
    if cl.is_dirty() or len(cl.get_entry(tag)) == 0:
        print ("Please fix your " + changelog)
        cleanup()
        sys.exit(1)

    with open("tmp.changelog", "w") as f:
        entry = cl.get_entry(tag)
        f.write(entry[0][3:] + "\n")
        f.write(entry[1])
    files.append("tmp.changelog")

    tarball_name = name+"-"+normalize_tag(tag)+".tar.gz"
    call(["git", "archive", "--format", "tar.gz",
         "--prefix", name+"-"+normalize_tag(tag)+"/", "-o",
          "export.tar.gz", tag])
    files.append("export.tar.gz")

    tarball_url = "https://codeload.github.com/"+user+"/"+name+"/tar.gz/"+tag
    print(tarball_url)
    urllib.request.urlretrieve(tarball_url, tarball_name)
    copyfile(tarball_name, "original.tar.gz")
    files.append(tarball_name)
    files.append("original.tar.gz")

    call(["gunzip", "original.tar.gz"])
    call(["gunzip", "export.tar.gz"])
    files.append("original.tar")
    files.append("export.tar")

    if call(["diffoscope", "original.tar", "export.tar"]) != 0:
        print("Retrieved tarball differs from local export")
        cleanup()
        sys.exit(1)

    files.append(tarball_name+".asc")
    if call(["gpg2", "--armor", "--detach-sign", tarball_name]) != 0:
        print("Error signing tarball")
        cleanup()
        sys.exit(1)

    if call(["hub",  "release", "create", "-f", "tmp.changelog", "-a",
             tarball_name+".asc", tag]) != 0:
        print("Error creating GitHub release")
    cleanup()


if __name__ == "__main__":
    main()
