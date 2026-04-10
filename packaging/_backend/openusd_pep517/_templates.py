from __future__ import annotations

from string import Template


BINARY_LAUNCHER_MODULE = """from __future__ import annotations

import os
import sys
import sysconfig
from pathlib import Path


def _library_env_key() -> str:
    if sys.platform == "win32":
        return "PATH"
    if sys.platform == "darwin":
        return "DYLD_FALLBACK_LIBRARY_PATH"
    return "LD_LIBRARY_PATH"


def _packaged_binary(tool_name: str) -> Path:
    package_root = Path(__file__).resolve().parent
    binary_dir = package_root / "bin"
    if sys.platform == "win32":
        executable = binary_dir / f"{tool_name}.exe"
        if executable.exists():
            return executable
    return binary_dir / tool_name


def _library_search_dirs(platlib: Path, launcher_package: Path) -> list[str]:
    search_dirs = []
    if launcher_package.exists():
        search_dirs.append(str(launcher_package))
    for libs_dir in sorted(platlib.glob("*.libs")):
        search_dirs.append(str(libs_dir))
    return search_dirs


def main() -> None:
    tool_name = Path(sys.argv[0]).name
    binary = _packaged_binary(tool_name)
    if not binary.exists():
        raise SystemExit(f"Could not find packaged OpenUSD tool: {binary}")

    env = os.environ.copy()
    platlib = Path(sysconfig.get_path("platlib"))
    search_dirs = _library_search_dirs(platlib, binary.parent)
    env_key = _library_env_key()
    if search_dirs:
        existing = env.get(env_key)
        if existing:
            search_dirs.append(existing)
        env[env_key] = os.pathsep.join(search_dirs)

    os.execvpe(str(binary), [str(binary), *sys.argv[1:]], env)
"""


BUILD_SYSTEM_PYPROJECT = """[build-system]
requires = ["setuptools>=69", "wheel"]
build-backend = "setuptools.build_meta"
"""


SETUP_PY_TEMPLATE = Template(
    """import re
import sysconfig
from pathlib import Path
from setuptools import find_namespace_packages, setup
from setuptools.command.bdist_wheel import bdist_wheel as _bdist_wheel
from setuptools.command.install import install as _install


class openusd_bdist_wheel(_bdist_wheel):
    def finalize_options(self):
        super().finalize_options()
        self.root_is_pure = False

    def get_tag(self):
        python_tag, abi_tag, platform_tag = super().get_tag()
        soabi = sysconfig.get_config_var("SOABI") or ""
        match = re.match(r"cpython-(\\d+[a-z]*)", soabi)
        if match:
            cpython_tag = f"cp{match.group(1)}"
            return cpython_tag, cpython_tag, platform_tag
        return python_tag, abi_tag, platform_tag


class openusd_install(_install):
    def finalize_options(self):
        super().finalize_options()
        self.install_lib = self.install_platlib


root = Path(__file__).resolve().parent
kwargs = $setup_kwargs_literal
packages = find_namespace_packages(where="staged/lib/python", include=["pxr*"])
for helper_package in $helper_packages_literal:
    if helper_package not in packages:
        packages.append(helper_package)
kwargs["packages"] = sorted(packages)
kwargs["long_description"] = (root / "README.md").read_text(encoding="utf-8")
kwargs["cmdclass"] = {
    "bdist_wheel": openusd_bdist_wheel,
    "install": openusd_install,
}

data_files = {}
for path in (root / "staged").rglob("*"):
    if not path.is_file():
        continue
    rel = path.relative_to(root / "staged")
    if rel.parts[:2] == ("lib", "python"):
        continue
    destination = "." if rel.parent == Path(".") else str(rel.parent)
    data_files.setdefault(destination, []).append(str(path))
kwargs["data_files"] = sorted((dst, files) for dst, files in data_files.items())
setup(**kwargs)
"""
)
