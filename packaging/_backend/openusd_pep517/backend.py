from __future__ import annotations

import base64
import contextlib
import csv
import hashlib
import io
import os
import platform
import pprint
import re
import runpy
import shutil
import subprocess
import sys
import tempfile
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python < 3.11
    import tomli as tomllib

REPO_ROOT = Path(__file__).resolve().parents[3]
PACKAGING_ROOT = REPO_ROOT / "packaging"
LOCAL_OPENUSD_DISTRIBUTIONS = ("usd-core", "usd-imaging", "usd-tools", "usdview")
PACKAGING_NOISE_PATTERNS = ("__pycache__", "*.pyc", "*.pyo")
OPENUSD_BUILD_SCRIPT = "build_scripts/build_usd.py"
OPENUSD_VERSION_FILE = "cmake/defaults/Version.cmake"
OPENUSD_PREBUILT_INSTALL_ENV = "OPENUSD_PREBUILT_INSTALL_ROOT"
OPENUSD_SKIP_REPAIR_ENV = "OPENUSD_SKIP_REPAIR"
OPENUSD_POST_RELEASE_ENV = "OPENUSD_POST_RELEASE_TAG"
SETUPTOOLS_PACKAGE_DATA_GLOBS = {
    "": [
        "*",
        "*/*",
        "*/*/*",
        "*/*/*/*",
        "*/*/*/*/*",
        "*/*/*/*/*/*",
        "*/*/*/*/*/*/*",
    ]
}
_TEMPLATES = runpy.run_path(Path(__file__).with_name("_templates.py"))
BUILD_SYSTEM_PYPROJECT = _TEMPLATES["BUILD_SYSTEM_PYPROJECT"]
BINARY_LAUNCHER_MODULE = _TEMPLATES["BINARY_LAUNCHER_MODULE"]
SETUP_PY_TEMPLATE = _TEMPLATES["SETUP_PY_TEMPLATE"]
WINDOWS_INIT_APPEND = """

# appended to this file for the windows PyPI package
import os, sys
dllPath = os.path.split(os.path.realpath(__file__))[0]
if sys.version_info >= (3, 8, 0):
    os.environ['PXR_USD_WINDOWS_DLL_PATH'] = dllPath
os.environ['PATH'] = dllPath + os.pathsep + os.environ['PATH']
"""


@dataclass
class ProjectMetadata:
    """Python package metadata loaded from the package-local pyproject.toml."""

    name: str
    description: str
    readme: str
    requires_python: str
    authors: list[dict[str, str]] = field(default_factory=list)
    urls: dict[str, str] = field(default_factory=dict)
    classifiers: list[str] = field(default_factory=list)
    dependencies: list[str] = field(default_factory=list)
    optional_dependencies: dict[str, list[str]] = field(default_factory=dict)
    license_text: str | None = None


@dataclass
class OpenUsdPackagingConfig:
    """OpenUSD-specific wheel build settings loaded from the pyproject tool table."""

    build_usd_args: list[str] = field(default_factory=list)
    windows_build_usd_extra_args: list[str] = field(default_factory=list)
    manifest_include: list[str] = field(default_factory=list)
    manifest_scripts: list[str] = field(default_factory=list)


@dataclass
class PackageConfig:
    """Complete backend configuration for one wheel-producing package directory."""

    project_root: Path
    metadata: ProjectMetadata
    packaging: OpenUsdPackagingConfig


@dataclass
class NormalizedInstallLayout:
    """Normalized install tree used as the source for manifest-based staging."""

    root: Path
    python_root: Path
    library_root: Path
    library_search_paths: list[str]


@dataclass
class StageLayout:
    """Manifest-filtered package payload and staged text scripts for wheel assembly."""

    package_root: Path
    runtime_python_root: Path
    runtime_library_root: Path
    runtime_library_search_paths: list[str]
    text_script_sources: list[Path]
    launcher_package_name: str | None = None
    launcher_script_names: list[str] = field(default_factory=list)


@dataclass
class DependencyWheel:
    """Previously built OpenUSD wheel that exports shared libraries to later wheels."""

    distribution_name: str
    libs_dir_name: str
    soname_to_vendored_name: dict[str, str]


def _run(
    command: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
) -> None:
    """Run a subprocess and echo the command for build logs."""

    print("+", " ".join(command))
    subprocess.run(command, cwd=cwd, env=env, check=True)


def _run_capture(
    command: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
) -> str:
    """Run a subprocess and return decoded stdout for small backend queries."""

    print("+", " ".join(command))
    return subprocess.check_output(command, cwd=cwd, env=env, text=True)


@contextlib.contextmanager
def _pushd(path: Path):
    """Temporarily change the working directory for local backend operations."""

    old = Path.cwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(old)


def _load_pyproject(project_root: Path) -> dict[str, Any]:
    """Load the package-local pyproject.toml file."""

    with (project_root / "pyproject.toml").open("rb") as fh:
        return tomllib.load(fh)


def _load_package_config(project_root: Path) -> PackageConfig:
    """Translate the package-local pyproject.toml into the backend's narrow config model."""

    data = _load_pyproject(project_root)
    project = data["project"]
    tool_cfg = data.get("tool", {}).get("openusd-packaging", {})
    build_cfg = tool_cfg.get("build", {})
    manifest_cfg = tool_cfg.get("manifest", {})

    metadata = ProjectMetadata(
        name=project["name"],
        description=project["description"],
        readme=project["readme"],
        requires_python=project["requires-python"],
        authors=project.get("authors", []),
        urls=project.get("urls", {}),
        classifiers=project.get("classifiers", []),
        dependencies=project.get("dependencies", []),
        optional_dependencies=project.get("optional-dependencies", {}),
        license_text=project["license"]["text"],
    )

    packaging = OpenUsdPackagingConfig(
        build_usd_args=build_cfg.get("args", []),
        windows_build_usd_extra_args=build_cfg.get("windows-extra-args", []),
        manifest_include=manifest_cfg.get("include", []),
        manifest_scripts=manifest_cfg.get("scripts", []),
    )

    return PackageConfig(project_root=project_root, metadata=metadata, packaging=packaging)


def _local_openusd_dependencies(distribution_name: str) -> list[str]:
    """Return transitive OpenUSD wheel dependencies in build order for one package."""

    ordered: list[str] = []
    seen: set[str] = set()

    def _visit(name: str) -> None:
        if name in seen or name not in LOCAL_OPENUSD_DISTRIBUTIONS:
            return
        seen.add(name)
        config = _load_package_config(PACKAGING_ROOT / name)
        for requirement in config.metadata.dependencies:
            match = re.match(r"\s*([A-Za-z0-9_.-]+)", requirement)
            dependency_name = match.group(1) if match else None
            if dependency_name in LOCAL_OPENUSD_DISTRIBUTIONS:
                _visit(dependency_name)
                if dependency_name not in ordered:
                    ordered.append(dependency_name)

    _visit(distribution_name)
    return ordered


def _parse_version(version_file: Path, post_release_tag: str | None) -> str:
    """Read the OpenUSD version from CMake metadata and map it to wheel versioning."""

    contents = version_file.read_text(encoding="utf-8")

    def _find(name: str) -> str:
        match = re.search(rf'set\(PXR_{name}_VERSION "([^"]+)"\)', contents)
        if not match:
            raise RuntimeError(f"Could not find PXR_{name}_VERSION in {version_file}")
        return match.group(1)

    minor = _find("MINOR")
    patch = _find("PATCH")
    version = f"{minor}.{patch}"
    if post_release_tag:
        version = f"{version}.{post_release_tag}"
    return version


def _is_packaging_noise(path: Path) -> bool:
    """Skip interpreter cache artifacts so wheel contents reflect staged sources only."""

    return "__pycache__" in path.parts or path.suffix.lower() in {".pyc", ".pyo"}


def _copy_tree(src: Path, dst: Path, *, merge: bool = False) -> None:
    """Copy a tree while filtering interpreter cache artifacts from staged payloads."""

    if not src.exists():
        return
    shutil.copytree(
        src,
        dst,
        dirs_exist_ok=merge,
        ignore=shutil.ignore_patterns(*PACKAGING_NOISE_PATTERNS),
    )


def _build_install_tree(config: PackageConfig, install_root: Path, work_root: Path) -> None:
    """Invoke build_usd.py to produce the install tree that wheel staging consumes."""

    args = [
        sys.executable,
        str(REPO_ROOT / OPENUSD_BUILD_SCRIPT),
        *config.packaging.build_usd_args,
        "--build",
        str(work_root / "build"),
        "--src",
        str(work_root / "src"),
        str(install_root),
        "-v",
    ]
    if platform.system() == "Windows":
        args.extend(config.packaging.windows_build_usd_extra_args)
    _run(args, cwd=REPO_ROOT)


def _prepare_install_tree(config: PackageConfig, work_root: Path) -> Path:
    """Either build OpenUSD or copy a prebuilt install tree into a temp work area."""

    install_root = work_root / "inst"
    prebuilt_root = os.environ.get(OPENUSD_PREBUILT_INSTALL_ENV)
    if prebuilt_root:
        print(f"Reusing prebuilt install root from {OPENUSD_PREBUILT_INSTALL_ENV}")
        prebuilt_install_root = Path(prebuilt_root)
        if not prebuilt_install_root.exists():
            raise RuntimeError(
                f"Prebuilt install root does not exist: {prebuilt_install_root}"
            )
        _copy_tree(prebuilt_install_root, install_root)
    else:
        _build_install_tree(config, install_root, work_root)
    return install_root


def _normalize_install_for_packaging(
    install_root: Path, normalized_root: Path
) -> NormalizedInstallLayout:
    """Rewrite the install tree into the stable layout expected by manifest staging."""

    lib_root = normalized_root / "lib"
    for subtree in ("lib", "bin", "plugin"):
        _copy_tree(install_root / subtree, normalized_root / subtree)

    plug_info_dir = normalized_root / "lib" / "python" / "pxr" / "pluginfo"
    plug_info_dir.parent.mkdir(parents=True, exist_ok=True)
    if (normalized_root / "lib" / "usd").exists():
        shutil.move(str(normalized_root / "lib" / "usd"), str(plug_info_dir))

    plugin_root = normalized_root / "plugin" / "usd"
    if plugin_root.exists():
        for plugin_dir in plugin_root.iterdir():
            if plugin_dir.is_dir():
                _copy_tree(plugin_dir, plug_info_dir / plugin_dir.name, merge=True)

    if platform.system() == "Windows":
        dll_files = list(normalized_root.glob("lib/*.dll"))
        dll_files.extend(normalized_root.glob("bin/*.dll"))
        pxr_dir = normalized_root / "lib" / "python" / "pxr"
        for dll in dll_files:
            shutil.move(str(dll), str(pxr_dir / dll.name))

        init_file = pxr_dir / "__init__.py"
        with init_file.open("a", encoding="utf-8") as fh:
            fh.write(WINDOWS_INIT_APPEND)

    library_dirs = set()
    for pattern in ("*.so", "*.so.*", "*.dylib", "*.dll", "*.pyd"):
        for path in normalized_root.rglob(pattern):
            if path.is_file():
                library_dirs.add(str(path.parent))

    return NormalizedInstallLayout(
        root=normalized_root,
        python_root=normalized_root / "lib" / "python",
        library_root=normalized_root / "lib",
        library_search_paths=sorted(library_dirs),
    )


def _iter_glob_files(root: Path, pattern: str) -> list[Path]:
    """Expand a manifest glob into a stable, de-duplicated list of staged files."""

    files: set[Path] = set()
    for match in root.glob(pattern):
        if match.is_dir():
            files.update(
                p
                for p in match.rglob("*")
                if p.is_file() and not _is_packaging_noise(p.relative_to(root))
            )
        elif match.is_file() and not _is_packaging_noise(match.relative_to(root)):
            files.add(match)
    return sorted(files)


def _collect_manifest_files(root: Path, patterns: list[str]) -> list[Path]:
    """Collect unique files from a list of manifest globs."""

    files: set[Path] = set()
    for pattern in patterns:
        files.update(_iter_glob_files(root, pattern))
    return sorted(files)


def _format_metadata_for_version(metadata: ProjectMetadata, version: str) -> ProjectMetadata:
    """Resolve version placeholders so inter-package dependencies match the wheel version."""

    return ProjectMetadata(
        name=metadata.name,
        description=metadata.description,
        readme=metadata.readme,
        requires_python=metadata.requires_python,
        authors=metadata.authors,
        urls=metadata.urls,
        classifiers=metadata.classifiers,
        dependencies=[dep.replace("{openusd_version}", version) for dep in metadata.dependencies],
        optional_dependencies={
            name: [dep.replace("{openusd_version}", version) for dep in deps]
            for name, deps in metadata.optional_dependencies.items()
        },
        license_text=metadata.license_text,
    )


def _distribution_module_name(distribution_name: str) -> str:
    """Convert a wheel distribution name into a stable Python helper-package name."""

    return re.sub(r"[^0-9A-Za-z_]+", "_", distribution_name)


def _load_linux_dependency_wheel(wheel_path: Path, distribution_name: str) -> DependencyWheel:
    """Read the vendored-library export map from one previously built Linux wheel."""

    libs_dir_name = ""
    soname_to_vendored_name: dict[str, str] = {}
    with zipfile.ZipFile(wheel_path) as wheel:
        for name in wheel.namelist():
            match = re.match(r"([^/]+\.libs)/(lib[^/]+\.so(?:\.[^/]+)?)$", name)
            if not match:
                continue
            libs_dir_name = match.group(1)
            vendored_name = match.group(2)
            soname = re.sub(r"-[0-9a-f]{8}(?=\.so(?:\.|$))", "", vendored_name)
            soname_to_vendored_name[soname] = vendored_name

    if not libs_dir_name:
        raise RuntimeError(
            f"Expected a repaired OpenUSD dependency wheel with a .libs directory: {wheel_path}"
        )

    return DependencyWheel(
        distribution_name=distribution_name,
        libs_dir_name=libs_dir_name,
        soname_to_vendored_name=soname_to_vendored_name,
    )


def _load_linux_dependency_wheels(
    distribution_name: str, version: str, wheel_directory: Path
) -> list[DependencyWheel]:
    """Load transitive Linux dependency wheels that should own shared libraries."""

    dependency_wheels: list[DependencyWheel] = []
    for dependency_name in _local_openusd_dependencies(distribution_name):
        pattern = f"{_distribution_module_name(dependency_name)}-{version}-*.whl"
        matching_wheels = sorted(
            wheel_directory.glob(pattern), key=lambda path: path.stat().st_mtime
        )
        if not matching_wheels:
            raise RuntimeError(
                "OpenUSD dependency wheel was not found for repair-time shared-library "
                f"ownership: {dependency_name} {version}. Build dependency wheels first."
            )
        dependency_wheels.append(
            _load_linux_dependency_wheel(matching_wheels[-1], dependency_name)
        )
    return dependency_wheels


def _is_elf_file(path: Path) -> bool:
    """Identify ELF payloads so Linux runtime patching only touches native binaries."""

    if not path.is_file():
        return False
    with path.open("rb") as fh:
        return fh.read(4) == b"\x7fELF"


def _wheel_record_hash(data: bytes) -> tuple[str, int]:
    """Compute the wheel RECORD hash tuple for one archive member."""

    digest = hashlib.sha256(data).digest()
    encoded = base64.urlsafe_b64encode(digest).rstrip(b"=").decode("utf-8")
    return f"sha256={encoded}", len(data)


def _rewrite_wheel_archive(source_root: Path, source_wheel: Path, target_wheel: Path) -> None:
    """Repack a wheel from an extracted tree while preserving the archive comment."""

    with zipfile.ZipFile(source_wheel, "r") as source_zip:
        compression = source_zip.compression
        comment = source_zip.comment
        info_map = {info.filename: info for info in source_zip.infolist()}

    with zipfile.ZipFile(target_wheel, "w", compression=compression) as wheel:
        wheel.comment = comment
        for path in sorted(source_root.rglob("*")):
            if not path.is_file():
                continue
            arcname = str(path.relative_to(source_root))
            source_info = info_map.get(arcname)
            if source_info is None:
                wheel.write(path, arcname=arcname)
                continue

            file_info = zipfile.ZipInfo(arcname, date_time=source_info.date_time)
            file_info.compress_type = source_info.compress_type
            file_info.comment = source_info.comment
            file_info.extra = source_info.extra
            file_info.create_system = source_info.create_system
            file_info.create_version = source_info.create_version
            file_info.extract_version = source_info.extract_version
            file_info.flag_bits = source_info.flag_bits
            file_info.internal_attr = source_info.internal_attr
            file_info.external_attr = source_info.external_attr
            wheel.writestr(file_info, path.read_bytes())


def _rewrite_wheel_record(wheel_path: Path) -> None:
    """Recompute wheel RECORD hashes after backend-side archive mutations."""

    with tempfile.TemporaryDirectory(prefix=f"{wheel_path.stem}-record-") as temp_dir:
        extract_root = Path(temp_dir) / "wheel"
        with zipfile.ZipFile(wheel_path, "r") as wheel:
            wheel.extractall(extract_root)

        record_path = next(extract_root.glob("*.dist-info/RECORD"))
        output = io.StringIO()
        writer = csv.writer(output, lineterminator="\n")
        for path in sorted(extract_root.rglob("*")):
            if not path.is_file():
                continue
            rel = str(path.relative_to(extract_root))
            if path == record_path:
                writer.writerow([rel, "", ""])
                continue
            digest, size = _wheel_record_hash(path.read_bytes())
            writer.writerow([rel, digest, str(size)])
        record_path.write_text(output.getvalue(), encoding="utf-8")

        rewritten_wheel = Path(temp_dir) / wheel_path.name
        _rewrite_wheel_archive(extract_root, wheel_path, rewritten_wheel)
        shutil.move(str(rewritten_wheel), wheel_path)


def _merge_rpath_entries(existing_rpath: str, new_entries: list[str]) -> str:
    """Append new RPATH entries while preserving existing order and de-duplicating."""

    merged: list[str] = []
    for entry in [*filter(None, existing_rpath.split(":")), *new_entries]:
        if entry and entry not in merged:
            merged.append(entry)
    return ":".join(merged)


def _patch_linux_wheel_dependency_links(
    wheel_path: Path, dependency_wheels: list[DependencyWheel]
) -> None:
    """Retarget excluded Linux SONAMEs to dependency wheels and extend RPATHs accordingly."""

    if not dependency_wheels:
        return

    dependency_map: dict[str, tuple[DependencyWheel, str]] = {}
    for dependency in dependency_wheels:
        for soname, vendored_name in dependency.soname_to_vendored_name.items():
            dependency_map[soname] = (dependency, vendored_name)

    patchelf_env = os.environ.copy()
    patchelf_env["PATH"] = os.pathsep.join(
        [str(Path(sys.executable).parent), patchelf_env.get("PATH", "")]
    )

    with tempfile.TemporaryDirectory(prefix=f"{wheel_path.stem}-patch-") as temp_dir:
        extract_root = Path(temp_dir) / "wheel"
        with zipfile.ZipFile(wheel_path, "r") as wheel:
            wheel.extractall(extract_root)

        for path in sorted(extract_root.rglob("*")):
            if not _is_elf_file(path):
                continue

            needed = [
                line.strip()
                for line in _run_capture(
                    ["patchelf", "--print-needed", str(path)], env=patchelf_env
                )
                .splitlines()
                if line.strip()
            ]
            matched_dependencies: dict[str, str] = {}
            for soname in needed:
                dependency_entry = dependency_map.get(soname)
                if not dependency_entry:
                    continue
                _, vendored_name = dependency_entry
                _run(
                    ["patchelf", "--replace-needed", soname, vendored_name, str(path)],
                    env=patchelf_env,
                )
                matched_dependencies[soname] = vendored_name

            if not matched_dependencies:
                continue

            rpath_entries = []
            for soname in matched_dependencies:
                dependency, _ = dependency_map[soname]
                relative = os.path.relpath(extract_root / dependency.libs_dir_name, path.parent)
                rpath_entries.append(f"$ORIGIN/{relative}")

            existing_rpath = _run_capture(
                ["patchelf", "--print-rpath", str(path)], env=patchelf_env
            )
            merged_rpath = _merge_rpath_entries(existing_rpath.strip(), rpath_entries)
            _run(
                ["patchelf", "--force-rpath", "--set-rpath", merged_rpath, str(path)],
                env=patchelf_env,
            )

        patched_wheel = Path(temp_dir) / wheel_path.name
        _rewrite_wheel_archive(extract_root, wheel_path, patched_wheel)
        shutil.move(str(patched_wheel), wheel_path)


def _write_launcher_package(package_stage_root: Path, package_name: str) -> Path:
    """Create the helper package that owns native tool payloads and the shared launcher."""

    launcher_root = package_stage_root / "lib" / "python" / package_name
    launcher_root.mkdir(parents=True, exist_ok=True)
    (launcher_root / "__init__.py").write_text(
        '"""Helper package for launching packaged OpenUSD native tools."""\n',
        encoding="utf-8",
    )
    (launcher_root / "launcher.py").write_text(
        BINARY_LAUNCHER_MODULE,
        encoding="utf-8",
    )
    return launcher_root


def _stage_manifest_package(
    config: PackageConfig, install_root: Path, work_root: Path
) -> StageLayout:
    """Apply manifest rules to the normalized install tree and prepare wheel inputs."""

    normalized_layout = _normalize_install_for_packaging(
        install_root, work_root / "normalized"
    )
    package_stage_root = work_root / "staged-package"
    package_stage_root.mkdir(parents=True, exist_ok=True)
    (package_stage_root / "lib" / "python").mkdir(parents=True, exist_ok=True)
    for src_path in _collect_manifest_files(
        normalized_layout.root, config.packaging.manifest_include
    ):
        rel = src_path.relative_to(normalized_layout.root)
        dst_path = package_stage_root / rel
        dst_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src_path, dst_path)
    script_sources = _collect_manifest_files(
        normalized_layout.root, config.packaging.manifest_scripts
    )
    text_script_sources = [path for path in script_sources if _is_text_script(path)]
    launcher_script_sources = [path for path in script_sources if not _is_text_script(path)]

    launcher_package_name: str | None = None
    launcher_script_names: list[str] = []
    if launcher_script_sources:
        launcher_package_name = (
            f"_openusd_launcher_{_distribution_module_name(config.metadata.name)}"
        )
        launcher_package_root = _write_launcher_package(
            package_stage_root, launcher_package_name
        )
        launcher_bin_root = launcher_package_root / "bin"
        launcher_bin_root.mkdir(parents=True, exist_ok=True)
        for src_path in launcher_script_sources:
            shutil.copy2(src_path, launcher_bin_root / src_path.name)
        launcher_script_names = [path.name for path in launcher_script_sources]

    return StageLayout(
        package_root=package_stage_root,
        runtime_python_root=normalized_layout.python_root,
        runtime_library_root=normalized_layout.library_root,
        runtime_library_search_paths=normalized_layout.library_search_paths,
        text_script_sources=text_script_sources,
        launcher_package_name=launcher_package_name,
        launcher_script_names=launcher_script_names,
    )


def _build_setup_py(
    metadata: ProjectMetadata,
    version: str,
    script_paths: list[str],
    launcher_package_name: str | None,
    launcher_script_names: list[str],
) -> str:
    """Generate the temporary setuptools project used to turn staged files into a wheel."""

    authors = metadata.authors
    author = authors[0]["name"] if authors else ""
    author_email = authors[0].get("email", "") if authors else ""
    helper_packages = [launcher_package_name] if launcher_package_name else []
    console_scripts = [
        f"{script_name}={launcher_package_name}.launcher:main"
        for script_name in launcher_script_names
    ]

    setup_kwargs = {
        "name": metadata.name,
        "version": version,
        "author": author,
        "author_email": author_email,
        "description": metadata.description,
        "long_description_content_type": "text/markdown",
        "license": metadata.license_text,
        "url": metadata.urls.get("Homepage", metadata.urls.get("Documentation", "")),
        "project_urls": metadata.urls,
        "packages": "__PACKAGES__",
        "package_dir": {"": "staged/lib/python"},
        "package_data": SETUPTOOLS_PACKAGE_DATA_GLOBS,
        "classifiers": metadata.classifiers,
        "python_requires": metadata.requires_python,
        "install_requires": metadata.dependencies,
        "extras_require": metadata.optional_dependencies,
        "scripts": script_paths,
        "entry_points": {"console_scripts": console_scripts},
        "include_package_data": False,
        "license_files": ["LICENSE.txt"],
    }
    setup_kwargs_literal = pprint.pformat(setup_kwargs, sort_dicts=True)
    helper_packages_literal = pprint.pformat(helper_packages, sort_dicts=True)

    return SETUP_PY_TEMPLATE.substitute(
        setup_kwargs_literal=setup_kwargs_literal,
        helper_packages_literal=helper_packages_literal,
    )


def _is_text_script(path: Path) -> bool:
    """Detect scripts that setuptools can install as text entry points instead of raw data."""

    data = path.read_bytes()[:4096]
    if data.startswith(b"#!"):
        return True
    if path.suffix.lower() in {".py", ".pyw", ".bat", ".cmd"}:
        return True
    return False


def _write_temp_project(
    config: PackageConfig,
    version: str,
    stage_layout: StageLayout,
    temp_project_root: Path,
) -> None:
    """Write the ephemeral setuptools project that packages one staged wheel payload."""

    readme_src = config.project_root / config.metadata.readme
    shutil.copy2(readme_src, temp_project_root / "README.md")
    shutil.copy2(REPO_ROOT / "LICENSE.txt", temp_project_root / "LICENSE.txt")
    shutil.copytree(stage_layout.package_root, temp_project_root / "staged")
    script_paths: list[str] = []
    if stage_layout.text_script_sources:
        scripts_root = temp_project_root / "scripts"
        scripts_root.mkdir(parents=True, exist_ok=True)
        for script_src in stage_layout.text_script_sources:
            shutil.copy2(script_src, scripts_root / script_src.name)
            script_paths.append(str(Path("scripts") / script_src.name))

    (temp_project_root / "pyproject.toml").write_text(
        BUILD_SYSTEM_PYPROJECT,
        encoding="utf-8",
    )
    metadata = _format_metadata_for_version(config.metadata, version)
    (temp_project_root / "setup.py").write_text(
        _build_setup_py(
            metadata,
            version,
            script_paths,
            stage_layout.launcher_package_name,
            stage_layout.launcher_script_names,
        ),
        encoding="utf-8",
    )


def _repair_linux(
    config: PackageConfig,
    version: str,
    raw_wheel: Path,
    stage_layout: StageLayout,
    wheel_directory: Path,
) -> str:
    """Run auditwheel and then rewrite plugInfo paths to match the repaired wheel layout."""

    dependency_wheels = _load_linux_dependency_wheels(
        config.metadata.name, version, wheel_directory
    )
    repair_dir = raw_wheel.parent / "repair"
    repair_dir.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["PATH"] = os.pathsep.join(
        [str(Path(sys.executable).parent), env.get("PATH", "")]
    )
    env["PYTHONPATH"] = str(stage_layout.runtime_python_root)
    ld_paths = list(stage_layout.runtime_library_search_paths) or [
        str(stage_layout.runtime_library_root)
    ]
    if env.get("LD_LIBRARY_PATH"):
        ld_paths.append(env["LD_LIBRARY_PATH"])
    env["LD_LIBRARY_PATH"] = os.pathsep.join(ld_paths)
    repair_command = ["auditwheel", "repair", "--wheel-dir", str(repair_dir)]
    excluded_sonames = sorted(
        {
            soname
            for dependency in dependency_wheels
            for soname in dependency.soname_to_vendored_name
        }
    )
    for soname in excluded_sonames:
        repair_command.extend(["--exclude", soname])
    repair_command.append(str(raw_wheel))
    _run(
        repair_command,
        env=env,
    )
    repaired_wheel = next(repair_dir.glob("*.whl"))
    _patch_linux_wheel_dependency_links(repaired_wheel, dependency_wheels)
    final_wheel = wheel_directory / repaired_wheel.name
    _run(
        [
            sys.executable,
            str(REPO_ROOT / "build_scripts/pypi/updatePluginfos.py"),
            str(repaired_wheel),
            str(final_wheel),
        ]
    )
    _rewrite_wheel_record(final_wheel)
    return final_wheel.name


def _repair_macos(
    raw_wheel: Path,
    install_root: Path,
    stage_layout: StageLayout,
    wheel_directory: Path,
) -> str:
    """Run delocate and then rewrite plugInfo paths to match the repaired wheel layout."""

    repair_dir = raw_wheel.parent / "repair"
    repair_dir.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["PATH"] = os.pathsep.join(
        [str(Path(sys.executable).parent), env.get("PATH", "")]
    )
    dyld_paths = list(stage_layout.runtime_library_search_paths) or [str(install_root / "lib")]
    env["DYLD_FALLBACK_LIBRARY_PATH"] = os.pathsep.join(dyld_paths)
    _run(
        ["delocate-wheel", "-w", str(repair_dir), str(raw_wheel)],
        env=env,
    )
    repaired_wheel = next(repair_dir.glob("*.whl"))
    final_wheel = wheel_directory / repaired_wheel.name
    _run(
        [
            sys.executable,
            str(REPO_ROOT / "build_scripts/pypi/updatePluginfos.py"),
            str(repaired_wheel),
            str(final_wheel),
        ]
    )
    return final_wheel.name


def _finalize_wheel(
    config: PackageConfig,
    version: str,
    raw_wheel: Path,
    stage_layout: StageLayout,
    install_root: Path,
    wheel_directory: Path,
) -> str:
    """Apply optional platform repair and copy the finished wheel to the requested output."""

    if os.environ.get(OPENUSD_SKIP_REPAIR_ENV) == "1":
        target = wheel_directory / raw_wheel.name
        shutil.copy2(raw_wheel, target)
        return target.name

    system = platform.system()
    if system == "Linux":
        return _repair_linux(config, version, raw_wheel, stage_layout, wheel_directory)
    if system == "Darwin":
        return _repair_macos(raw_wheel, install_root, stage_layout, wheel_directory)

    target = wheel_directory / raw_wheel.name
    shutil.copy2(raw_wheel, target)
    return target.name


def get_requires_for_build_wheel(config_settings: dict[str, Any] | None = None) -> list[str]:
    """Report only the platform repair tools needed by the active wheel configuration."""

    if os.environ.get(OPENUSD_SKIP_REPAIR_ENV) == "1":
        return []

    system = platform.system()
    if system == "Linux":
        return ["auditwheel", "patchelf"]
    if system == "Darwin":
        return ["delocate==0.13.0"]
    return []


def build_wheel(
    wheel_directory: str,
    config_settings: dict[str, Any] | None = None,
    metadata_directory: str | None = None,
) -> str:
    """PEP 517 entry point for wheel builds from a package-local pyproject directory."""

    config = _load_package_config(Path.cwd())
    wheel_dir = Path(wheel_directory).resolve()
    wheel_dir.mkdir(parents=True, exist_ok=True)

    post_release_tag = os.environ.get(OPENUSD_POST_RELEASE_ENV, "").strip()
    version = _parse_version(REPO_ROOT / OPENUSD_VERSION_FILE, post_release_tag)

    with tempfile.TemporaryDirectory(prefix=f"{config.metadata.name}-") as temp_dir:
        work_root = Path(temp_dir)
        install_root = _prepare_install_tree(config, work_root)
        stage_layout = _stage_manifest_package(config, install_root, work_root)

        temp_project_root = work_root / "project"
        temp_project_root.mkdir(parents=True, exist_ok=True)
        _write_temp_project(config, version, stage_layout, temp_project_root)

        raw_wheel_dir = work_root / "raw-dist"
        raw_wheel_dir.mkdir(parents=True, exist_ok=True)
        try:
            import setuptools.build_meta as build_meta
        except ModuleNotFoundError as exc:
            raise RuntimeError(
                "setuptools.build_meta is required in the PEP 517 build environment. "
                "It should be provided by build-system.requires."
            ) from exc

        with _pushd(temp_project_root):
            raw_name = build_meta.build_wheel(
                str(raw_wheel_dir), config_settings, metadata_directory
            )

        return _finalize_wheel(
            config, version, raw_wheel_dir / raw_name, stage_layout, install_root, wheel_dir
        )


def build_sdist(
    sdist_directory: str,
    config_settings: dict[str, Any] | None = None,
) -> str:
    """Explicitly reject sdists until the packaging migration has a reviewed source layout."""

    raise NotImplementedError(
        "OpenUSD's local backend does not build sdists yet; wheel builds are the "
        "supported path during the initial pyproject migration."
    )
