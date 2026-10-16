# OpenUSD Python Bindings Benchmarks (Boost.Python vs. nanobind)

This directory contains [`benchmark_usd.py`](./benchmark_usd.py), a benchmark suite exercising core OpenUSD Python binding functionality across `Gf`, `Vt`, `Sdf`, `Usd`, `UsdGeom`, `UsdShade`, `UsdLux`, and `UsdPhysics`.

## Usage

### Single Environment
Run against the currently active `PYTHONPATH` / `LD_LIBRARY_PATH`:

```bash
python nanobind_benchmarks/benchmark_usd.py
```

### Side-by-Side Comparison (Boost.Python vs. nanobind)
Compare two OpenUSD install prefixes directly (runs each in an isolated subprocess with its own `PYTHONPATH` and `LD_LIBRARY_PATH`):

```bash
python nanobind_benchmarks/benchmark_usd.py \
  --boost-prefix /usr/local/google/home/vicini/.local \
  --nanobind-prefix /usr/local/google/home/vicini/code/OpenUSD/install \
  --json-out nanobind_benchmarks/results.json
```

## 1. Runtime Benchmarks (Python 3.13.15, Linux x86_64)

| Benchmark | Boost.Python Median (ms) | nanobind Median (ms) | Speedup |
| :--- | ---: | ---: | ---: |
| Gf.Vec3d math (50k ops) | 305.77 | 57.67 | **5.30x** |
| Gf.Matrix4d transform & inverse (30k ops) | 91.23 | 20.45 | **4.46x** |
| Gf.Range3d & Gf.Quatd (30k ops) | 81.40 | 23.07 | **3.53x** |
| VtArray construction & indexing (2k arrays) | 46.65 | 9.12 | **5.11x** |
| Sdf.Path manipulation (40k ops) | 79.88 | 41.21 | **1.94x** |
| Sdf.Layer PrimSpec authoring (2k prims) | 26.07 | 14.89 | **1.75x** |
| Usd.Stage define & traverse (2k prims) | 123.21 | 94.21 | **1.31x** |
| Usd.Attribute time-samples set/get (5k samples) | 40.52 | 33.81 | **1.20x** |
| UsdGeom XformCache & BBoxCache (1k prims) | 40.42 | 38.89 | **1.04x** |
| UsdShade Material & Shader binding (500 mats) | 159.30 | 145.89 | **1.09x** |
| UsdLux & UsdPhysics schema authoring (800 prims) | 225.52 | 199.17 | **1.13x** |
| **Geometric Mean** | | | **2.07x** |

## 2. Binary Size Comparison (`pxr/*/_*.so`)

| Category | Boost.Python | nanobind | Ratio (bp/nb) | Change |
| :--- | ---: | ---: | ---: | ---: |
| **All 41 Python modules (unstripped)** | 108.77 MB | 46.03 MB | **2.36x** | **-57.7%** |
| **All 41 Python modules (stripped)** | 71.11 MB | 30.88 MB | **2.30x** | **-56.6%** |
| `_vt.so` | 24.02 MB | 13.32 MB | **1.80x** | **-44.5%** |
| `_sdf.so` | 17.80 MB | 7.49 MB | **2.37x** | **-57.9%** |
| `_gf.so` | 12.11 MB | 4.59 MB | **2.64x** | **-62.1%** |
| `_usd.so` | 8.64 MB | 3.17 MB | **2.73x** | **-63.3%** |
| `_usdPhysics.so` | 7.89 MB | 1.43 MB | **5.51x** | **-81.9%** |
| `_usdGeom.so` | 5.23 MB | 2.30 MB | **2.27x** | **-56.0%** |
| `_tf.so` | 3.15 MB | 1.21 MB | **2.60x** | **-61.5%** |
| `_usdShade.so` | 2.96 MB | 1.03 MB | **2.87x** | **-65.1%** |
| `_pcp.so` | 2.51 MB | 0.88 MB | **2.84x** | **-64.8%** |
| `_usdLux.so` | 2.46 MB | 1.05 MB | **2.35x** | **-57.5%** |

## 3. Build Time Comparison (Clang 19.1.7, `-O3 -DNDEBUG`, 24 Cores)

| Scope | Boost.Python | nanobind | Speedup / Change |
| :--- | ---: | ---: | ---: |
| **Clean Python bindings wall time (`-j 24`)** | 154.40 s | 135.01 s | **1.14x (-12.6%)** |
| **Clean Python bindings CPU time (`user + sys`)** | 3,109.06 s | 3,017.56 s | **1.03x (-2.9%)** |
| **Binding runtime library (`libusd_python` vs `nanobind/src`)** | 36.1 s (27 TUs) | 19.5 s (12 TUs) | **1.85x (-46.0%)** |
| `_vt` wrapper TUs (16 files) | 266.0 s | 223.3 s | **1.19x (-16.0%)** |
| `_usdPhysics` wrapper TUs (21 files) | 179.8 s | 149.1 s | **1.21x (-17.1%)** |
| `_usd` wrapper TUs (46 files) | 322.1 s | 300.8 s | **1.07x (-6.6%)** |
| `_sdf` wrapper TUs (37 files) | 261.9 s | 244.9 s | **1.07x (-6.5%)** |
| `_usdGeom` wrapper TUs (40 files) | 314.9 s | 296.2 s | **1.06x (-5.9%)** |
