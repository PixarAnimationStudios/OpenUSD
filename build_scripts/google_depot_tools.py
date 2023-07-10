# Copyright 2023 The Dawn & Tint Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
Helper script to download source dependencies without the need to
install depot_tools manually. This script implements a subset of
`gclient sync`.

This helps embedders, for example through CMake, get all the sources with
a single add_subdirectory call (or FetchContent) instead of more complex setups

Note that this script executes blindly the content of DEPS file, run it only on
a project that you trust not to contain malicious DEPS files.

Modified version of https://source.chromium.org/chromium/chromium/src/+/main:third_party/dawn/tools/fetch_dawn_dependencies.py
"""

import os
import subprocess
from pathlib import Path
import os

def fetch_dependecies(required_submodules):
    root_dir = Path("").resolve()

    process_dir(root_dir, required_submodules)


def process_dir(dir_path, required_submodules):
    """
    Install dependencies for the provided directory by processing the DEPS file
    that it contains (if it exists).
    Recursively install dependencies in sub-directories that are created by
    cloning dependencies.
    """
    deps_path = dir_path / 'DEPS'
    if not deps_path.is_file():
        return

    log(f"Listing dependencies from {dir_path}")
    DEPS = open(deps_path).read()

    ldict = {}
    exec(DEPS, globals(), ldict)
    deps = ldict.get('deps')
    variables = ldict.get('vars', {})

    if deps is None:
        log(f"ERROR: DEPS file '{deps_path}' does not define a 'deps' variable"
            )
        exit(1)

    for submodule in required_submodules:
        if submodule not in deps:
            continue
        submodule_path = dir_path / Path(submodule)

        raw_url = deps[submodule]['url']
        git_url, git_tag = raw_url.format(**variables).rsplit('@', 1)

        # Run git from within the submodule's path (don't use for clone)
        git = lambda *x: subprocess.run(["git", '-C', str(Path(submodule_path).absolute()), *x],
                                        capture_output=True)
        
        log(f"Fetching dependency '{submodule}'")
        
        if not submodule_path.is_dir():
            log(f"Shallow cloning '{git_url}' at '{git_tag}' into '{submodule_path}'"
                )
            shallow_clone(git, git_url, git_tag, submodule_path)

            log(f"Checking out tag '{git_tag}'")
            git('checkout', git_tag)

        elif (submodule_path / ".git").is_dir():
            # The module was already cloned, but we may need to update it
            proc = git('rev-parse', 'HEAD')
            need_update = proc.stdout.decode().strip() != git_tag

            if need_update:
                # The module was already cloned, but we may need to update it
                proc = git('cat-file', '-t', git_tag)
                git_tag_exists = proc.returncode == 0

                if not git_tag_exists:
                    log(f"Updating '{submodule_path}' from '{git_url}'")
                    git('fetch', 'origin', git_tag, '--depth', '1')

                log(f"Checking out tag '{git_tag}'")
                git('checkout', git_tag)

        else:
            # The caller may have "flattened" the source tree to get rid of
            # some heavy submodules.
            log(f"(Overridden by a local copy of the submodule)")

        # Recursive call
        required_subsubmodules = [
            m[len(submodule) + 1:] for m in required_submodules
            if m.startswith(submodule + "/")
        ]
        process_dir(submodule_path, required_subsubmodules)


def shallow_clone(git, git_url, git_tag, submodule_path):
    """
    Fetching only 1 commit is not exposed in the git clone API, so we decompose
    it manually in git init, git fetch, git reset.
    """
    submodule_path.mkdir()
    git('init')
    git('remote', 'add', 'origin', git_url)
    git('fetch', 'origin', git_tag, '--depth', '1')


def log(msg):
    """Just makes it look good in the CMake log flow."""
    print(f"-- -- {msg}")


class Var:
    """
    Mock Var class, that the content of DEPS files assume to exist when they
    are exec-ed.
    """
    def __init__(self, name):
        self.name = name

    def __add__(self, text):
        return self.name + text

    def __radd__(self, text):
        return text + self.name

