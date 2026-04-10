from __future__ import annotations

import runpy
from pathlib import Path

_exports = runpy.run_path(
    Path(__file__).resolve().parents[2]
    / "_backend"
    / "openusd_pep517"
    / "in_tree_backend_shim.py",
    init_globals={"_PACKAGE_BACKEND_FILE": __file__},
)

build_sdist = _exports["build_sdist"]
build_wheel = _exports["build_wheel"]
get_requires_for_build_wheel = _exports["get_requires_for_build_wheel"]
