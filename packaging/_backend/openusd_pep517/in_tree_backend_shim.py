from __future__ import annotations

import importlib.util
import sys
from pathlib import Path


def _load_shared_backend(package_backend_file: str):
    """Load the shared backend module while keeping backend-path inside each package tree."""

    shared_backend = (
        Path(package_backend_file).resolve().parents[2]
        / "_backend"
        / "openusd_pep517"
        / "backend.py"
    )
    spec = importlib.util.spec_from_file_location(
        "_openusd_pep517_shared_backend", shared_backend
    )
    if spec is None or spec.loader is None:
        raise ImportError(f"Could not load shared backend from {shared_backend}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


_package_backend_file = globals().get("_PACKAGE_BACKEND_FILE")
if not _package_backend_file:
    raise RuntimeError("_PACKAGE_BACKEND_FILE must be set before loading the shim")

_SHARED_BACKEND = _load_shared_backend(_package_backend_file)

build_sdist = _SHARED_BACKEND.build_sdist
build_wheel = _SHARED_BACKEND.build_wheel
get_requires_for_build_wheel = _SHARED_BACKEND.get_requires_for_build_wheel
