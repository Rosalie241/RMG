#!/usr/bin/env python3
import sys
import os
import subprocess
def system(cmd, default):
    args = cmd.split()
    try:
        return subprocess.check_output(args).decode("ascii").strip()
    except subprocess.CalledProcessError:
        return default
if __name__ == "__main__":
    hash = system("git show -s --format=%h", "0" * 40)
    mappings = {
        "GIT_COMMIT_HASH": hash,
    }
    base_path = os.path.dirname(os.path.abspath(__file__))
    core_path = os.path.join(base_path)
    in_path = os.path.join(core_path, "VersionHash.hpp.in")
    out_path = os.path.join(core_path, "VersionHash.hpp")
    with open(in_path, "r") as f:
        version_str = f.read()
    for mapping, value in mappings.items():
        version_str = version_str.replace("@" + mapping + "@", value)
    with open(out_path, "w") as f:
        f.write(version_str)