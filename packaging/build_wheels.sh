#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: packaging/build_wheels.sh [options]

Creates a virtual environment, installs the Python packaging prerequisites,
builds a shared OpenUSD install tree, and then builds one or more wheels from
that install tree. When `usdview` is requested, the helper also installs the
Qt-for-Python build dependencies needed by `build_usd.py`, including the
matching `pyside*-uic` executable in the venv.

Options:
  --python PATH            Python interpreter to use for the venv (default: python3)
  --venv DIR               Virtualenv directory (default: <repo>/.venv_openusd_packaging)
  --work-root DIR          Scratch/work directory (default: <repo>/.wheel-build)
  --install-root DIR       Prebuilt install tree path (default: <work-root>/install)
  --wheel-dir DIR          Output directory for wheels (default: <repo>/dist/wheels)
  --packages LIST          Comma-separated package list. Supported values:
                           usd-core,usd-imaging,usd-tools,usdview
                           Default: all four packages
  --build-usd-arg ARG      Extra argument passed through to build_usd.py.
                           May be repeated.
  --reuse-install          Reuse an existing install tree instead of rebuilding it
  --skip-repair            Set OPENUSD_SKIP_REPAIR=1 while building wheels
  --post-release-tag TAG   Set OPENUSD_POST_RELEASE_TAG for wheel builds
  -h, --help               Show this message

Examples:
  packaging/build_wheels.sh
  packaging/build_wheels.sh --packages usd-core,usd-tools
  packaging/build_wheels.sh --reuse-install --install-root /tmp/openusd-inst
EOF
}

# Return success when a named wheel package is in the requested package list.
contains_package() {
    local needle="$1"
    local pkg
    for pkg in "${packages[@]}"; do
        if [[ "$pkg" == "$needle" ]]; then
            return 0
        fi
    done
    return 1
}

# Fail early when the selected interpreter lacks the headers or libpython OpenUSD needs.
require_python_dev_artifacts() {
    local python_include
    local python_platinclude
    local python_libdir
    local python_ldlibrary
    local python_instsoname
    local python_version_short
    local -a python_dev_info=()
    local missing=()
    local found_library=""

    mapfile -t python_dev_info < <(python - <<'PY'
import sysconfig
import sys
print(sysconfig.get_path("include") or "")
print(sysconfig.get_path("platinclude") or "")
print(sysconfig.get_config_var("LIBDIR") or "")
print(sysconfig.get_config_var("LDLIBRARY") or "")
print(sysconfig.get_config_var("INSTSONAME") or "")
print(f"{sys.version_info.major}.{sys.version_info.minor}")
PY
)
    python_include="${python_dev_info[0]:-}"
    python_platinclude="${python_dev_info[1]:-}"
    python_libdir="${python_dev_info[2]:-}"
    python_ldlibrary="${python_dev_info[3]:-}"
    python_instsoname="${python_dev_info[4]:-}"
    python_version_short="${python_dev_info[5]:-}"

    if [[ -z "$python_include" || ! -f "${python_include}/Python.h" || ! -f "${python_include}/patchlevel.h" ]]; then
        missing+=("headers in ${python_include:-<unknown>}")
    fi

    if [[ -n "$python_platinclude" && ! -f "${python_platinclude}/pyconfig.h" ]]; then
        missing+=("pyconfig.h in ${python_platinclude}")
    elif [[ -z "$python_platinclude" && ( -z "$python_include" || ! -f "${python_include}/pyconfig.h" ) ]]; then
        missing+=("pyconfig.h")
    fi

    if [[ -n "$python_libdir" ]]; then
        if [[ -n "$python_ldlibrary" && -f "${python_libdir}/${python_ldlibrary}" ]]; then
            found_library="${python_libdir}/${python_ldlibrary}"
        elif [[ -n "$python_instsoname" && -f "${python_libdir}/${python_instsoname}" ]]; then
            found_library="${python_libdir}/${python_instsoname}"
        fi
    fi

    if [[ -z "$found_library" ]]; then
        missing+=("libpython in ${python_libdir:-<unknown>}")
    fi

    if [[ "${#missing[@]}" -gt 0 ]]; then
        echo "The selected Python interpreter is missing required development artifacts:" >&2
        printf '  - %s\n' "${missing[@]}" >&2
        echo >&2
        echo "OpenUSD needs a Python with headers and libpython available." >&2
        echo "On distro Python installs, this usually means installing python3-dev or python${python_version_short}-dev." >&2
        echo "A non-system Python that bundles headers and libpython also works." >&2
        echo "The packaging helper already supports that via --python /path/to/python." >&2
        exit 1
    fi
}

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
python_bin="${PYTHON:-python3}"
venv_dir="${repo_root}/.venv_openusd_packaging"
work_root="${repo_root}/.wheel-build"
install_root="${work_root}/install"
wheel_dir="${repo_root}/dist/wheels"
packages=(usd-core usd-imaging usd-tools usdview)
extra_build_usd_args=()
reuse_install=0
skip_repair=0
post_release_tag=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --python)
            python_bin="$2"
            shift 2
            ;;
        --venv)
            venv_dir="$2"
            shift 2
            ;;
        --work-root)
            work_root="$2"
            install_root="${work_root}/install"
            shift 2
            ;;
        --install-root)
            install_root="$2"
            shift 2
            ;;
        --wheel-dir)
            wheel_dir="$2"
            shift 2
            ;;
        --packages)
            IFS=',' read -r -a packages <<< "$2"
            shift 2
            ;;
        --build-usd-arg)
            extra_build_usd_args+=("$2")
            shift 2
            ;;
        --reuse-install)
            reuse_install=1
            shift
            ;;
        --skip-repair)
            skip_repair=1
            shift
            ;;
        --post-release-tag)
            post_release_tag="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

for pkg in "${packages[@]}"; do
    case "$pkg" in
        usd-core|usd-imaging|usd-tools|usdview) ;;
        *)
            echo "Unsupported package: $pkg" >&2
            exit 1
            ;;
    esac
done

system_name="$(uname -s)"

mkdir -p "$wheel_dir" "$work_root"

if [[ ! -d "$venv_dir" ]]; then
    "$python_bin" -m venv "$venv_dir"
fi

# shellcheck disable=SC1091
source "${venv_dir}/bin/activate"

python -m pip install --upgrade pip

pip_packages=(
    "build"
    "setuptools>=69"
    "wheel"
    "tomli; python_version < '3.11'"
)

if [[ "$skip_repair" -eq 0 ]]; then
    if [[ "$system_name" == "Linux" ]]; then
        pip_packages+=("auditwheel" "patchelf")
    elif [[ "$system_name" == "Darwin" ]]; then
        pip_packages+=("delocate==0.13.0")
    fi
fi

if contains_package "usd-tools"; then
    pip_packages+=("Jinja2")
fi

usdview_cmake_args=()
if contains_package "usdview"; then
    pip_packages+=("PyOpenGL")
    if python -c 'import sys; raise SystemExit(0 if sys.version_info < (3, 11) else 1)'; then
        pip_packages+=("PySide2==5.15.2.1" "shiboken2==5.15.2.1")
        usdview_cmake_args+=("-DPYSIDE_USE_PYSIDE2=ON")
    else
        pip_packages+=("PySide6==6.11.0" "shiboken6==6.11.0")
    fi
fi

python -m pip install "${pip_packages[@]}"

require_python_dev_artifacts

if [[ "$skip_repair" -eq 0 && "$system_name" == "Linux" ]]; then
    if ! command -v patchelf >/dev/null 2>&1; then
        echo "patchelf was not found in PATH after installation." >&2
        echo "The Linux auditwheel repair step requires the patchelf executable." >&2
        exit 1
    fi
fi

if contains_package "usdview"; then
    pyside_uic_candidates=("pyside6-uic" "python3-pyside6-uic")
    if [[ "${#usdview_cmake_args[@]}" -gt 0 ]]; then
        pyside_uic_candidates=("pyside2-uic")
    fi

    found_pyside_uic=""
    for pyside_uic in "${pyside_uic_candidates[@]}"; do
        if command -v "$pyside_uic" >/dev/null 2>&1; then
            found_pyside_uic="$pyside_uic"
            break
        fi
    done

    if [[ -z "$found_pyside_uic" ]]; then
        echo "Did not find a PySide uic binary in PATH after installation." >&2
        echo "Expected one of: ${pyside_uic_candidates[*]}" >&2
        exit 1
    fi
fi

if [[ "$reuse_install" -eq 0 ]]; then
    cmake_cache="${work_root}/build/OpenUSD/CMakeCache.txt"
    if [[ -f "$cmake_cache" ]] && rg -q '^PXR_PY_UNDEFINED_DYNAMIC_LOOKUP(:[^=]+)?=ON$' "$cmake_cache"; then
        echo "Detected a stale CMake cache with PXR_PY_UNDEFINED_DYNAMIC_LOOKUP=ON:" >&2
        echo "  $cmake_cache" >&2
        echo "This breaks native tool links when building the packaging install tree." >&2
        echo "Use a fresh --work-root or remove ${work_root}/build before retrying." >&2
        exit 1
    fi

    usd_cmake_args="-DPXR_BUILD_EXEC=OFF -DPXR_INSTALL_LOCATION=../pxr/pluginfo"
    if ! contains_package "usd-tools"; then
        usd_cmake_args+=" -DPXR_BUILD_USD_TOOLS=OFF"
    fi
    if [[ "${#usdview_cmake_args[@]}" -gt 0 ]]; then
        usd_cmake_args+=" ${usdview_cmake_args[*]}"
    fi

    mkdir -p "$install_root"
    build_args=(
        "--build-args"
        "USD,${usd_cmake_args}"
        "--no-materialx"
        "--no-examples"
        "--no-tutorials"
        "--build"
        "${work_root}/build"
        "--src"
        "${work_root}/src"
    )

    if ! contains_package "usd-imaging" && ! contains_package "usdview"; then
        build_args+=("--no-imaging")
    fi

    if ! contains_package "usdview"; then
        build_args+=("--no-usdview")
    fi

    python "${repo_root}/build_scripts/build_usd.py" \
        "${build_args[@]}" \
        "${extra_build_usd_args[@]}" \
        "$install_root" \
        -v
elif [[ ! -d "$install_root" ]]; then
    echo "Install root does not exist: $install_root" >&2
    exit 1
fi

export OPENUSD_PREBUILT_INSTALL_ROOT="$install_root"

if [[ "$skip_repair" -eq 1 ]]; then
    export OPENUSD_SKIP_REPAIR=1
else
    unset OPENUSD_SKIP_REPAIR || true
fi

if [[ -n "$post_release_tag" ]]; then
    export OPENUSD_POST_RELEASE_TAG="$post_release_tag"
else
    unset OPENUSD_POST_RELEASE_TAG || true
fi

for pkg in "${packages[@]}"; do
    echo "Building wheel for ${pkg}"
    python -m build --wheel --no-isolation --outdir "$wheel_dir" "${repo_root}/packaging/${pkg}"
done

echo
echo "Wheels written to ${wheel_dir}"
ls -1 "$wheel_dir"
