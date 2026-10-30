# OpenUSD Migration from `Boost.Python` to `nanobind`

## Executive Summary

This report documents the full migration of OpenUSD's Python bindings (`42` extension modules, `~650` files) from `pxr_boost::python` (`pxr/external/boost`) to [`nanobind`](https://github.com/wjakob/nanobind) (`pxr/external/nanobind`), along with the complete removal of `libusd_python.so` and `libusd_boost.so`.

Across the full OpenUSD test suite (`1,011 / 1,011` CTest targets passing at 100%):
- **Runtime Performance**: **2.07x geometric mean speedup** across 11 representative USD/Gf/Vt/Sdf/UsdGeom/UsdShade/UsdPhysics workloads (up to **5.30x** faster on `Gf.Vec3d` math and **5.11x** faster on `VtArray` operations).
- **Binary Size**: **57.7% reduction (2.36x smaller)** across all 41 common `_*.so` Python extension modules (`108.77 MB` $\rightarrow$ `46.03 MB` unstripped; `71.11 MB` $\rightarrow$ `30.88 MB` stripped).
- **Build Time**: **12.6% faster (1.14x)** wall-clock compilation time for all 545 Python wrapper translation units + binding runtime (`154.40 s` $\rightarrow$ `135.01 s` on 24 cores), and **46.0% faster (1.85x)** for the binding runtime itself.

---

## 1. Advantages & Disadvantages of Moving to `nanobind`

### Advantages
1. **Substantially Lower Call Overhead (`2.07x` GeoMean, up to `5.3x` on Math/Array Types)**
   - `nanobind` uses Python 3.8+ `METH_FASTCALL` / `vectorcall` calling conventions directly instead of constructing intermediate `PyTuple` and `PyDict` argument objects on every function call.
   - Compact per-instance object layout (`nb_inst`) avoids `Boost.Python`'s heavy `instance<>` + `value_holder` heap allocations and registry lookups.
2. **Dramatically Smaller Binaries (`-57.7%` / `-62.7 MB` Across Python Modules)**
   - `Boost.Python` instantiates deep template call chains, signature metadata, and converter registration structs per bound function/overload.
   - `nanobind` factors type-erased function dispatch, overload resolution, and lifetime management into a tiny shared C++ runtime (~12 source files compiled directly into `libusd_tf.so`), shrinking modules like `_usdPhysics.so` by **81.9% (`5.51x`)** and `_usd.so` by **63.3% (`2.73x`)**.
3. **Elimination of `pxr/external/boost` (`-45,049` Lines of Legacy Code)**
   - Removes 459 files of forked, C++03-era `pxr_boost::python` headers and sources, eliminating `libusd_python.so` and `libusd_boost.so` from the build and installation tree.
4. **Modern C++17 / Python 3.12+ / Free-Threading Readiness**
   - Native support for C++17 types (`std::optional`, `std::variant`, `std::string_view`, smart pointers), PEP 3118 buffer protocol / `ndarray`, type annotations, and upstream Python 3.13+ free-threading (`nb_ft.cpp`).
5. **Faster Compilation & Cleaner Diagnostics**
   - Lighter header weight and shallower template instantiation depth reduce compiler memory footprint and cut clean Python bindings wall-clock build time by **12.6%** (`-16.0%` on `_vt`, `-17.1%` on `_usdPhysics`).

### Disadvantages & Risks
1. **External C++ Plugin / Downstream Binding Breakage**
   - Third-party C++ plugins that write their own `wrap*.cpp` files using `pxr/base/tf/py*.h` or `usdGenSchema` templates will need to update their wrapper files from `pxr_boost::python` to `nanobind` when upgrading to this USD version. (Pure Python consumers of `from pxr import ...` are unaffected.)
2. **Stricter Overload Resolution & Implicit Conversion Semantics**
   - `nanobind` uses a two-pass overload dispatcher (Pass 1: exact type match without implicit conversions; Pass 2: implicit conversions enabled).
   - Overloads that previously relied on `Boost.Python`'s LIFO registration order or unrestricted implicit conversions required careful ordering or custom `type_caster` logic (e.g., `UsdStage::Open(layer, ...)` vs. `UsdStage::Open(filePath, ...)` when passed `ArResolverContext`).
3. **Lack of Built-in Global Call Hooks**
   - Unlike `Boost.Python`, `nanobind` intentionally avoids per-call pre/post policy hooks in its C++ template layer to keep generated code small. Supporting OpenUSD's automatic `TfErrorMark` $\rightarrow$ Python exception conversion required hooking `vectorcall` at the Python type-slot level in `Tf` rather than via a `nanobind` call policy.

---

## 2. Key Architectural Changes & Tradeoffs (Beyond Mechanical Porting)

While roughly 80% of the diff across `wrap*.cpp` files is mechanical (`class_<T>` $\rightarrow$ `nb::class_<T>`, `def(...) .staticmethod(...)` $\rightarrow$ `def_static(...)`, `add_property` $\rightarrow$ `def_prop_ro` / `def_prop_rw`, `self_ns::str(self)` $\rightarrow$ `TfPyRepr`), several foundational subsystems in `pxr/base/tf`, `pxr/base/vt`, `pxr/usd/sdf`, and `pxr/usd/usd` required custom architectural solutions:

### 2.1 Unmodified Upstream `nanobind` Runtime in `libusd_tf.so`
- **Design**: `pxr/external/nanobind` is 100% unmodified upstream `nanobind` (`da42c8a5890eff09e672e7cfd953627655aa3e28`) + `tsl-robin-map` (`v1.3.0`). Instead of building a separate `libusd_nanobind.so` library (replacing `libusd_python.so` + `libusd_boost.so`), the 12 `pxr/external/nanobind/src/*.cpp` runtime files are compiled directly into `libusd_tf.so` with `NB_BUILD=1`, and `tf` exports `NB_SHARED=1` publicly. [`pxr/base/tf/pySafePython.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pySafePython.h) and [`pxr/base/tf/pyModule.cpp`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyModule.cpp) route `NB_CTX` to a single shared `nb_internals*` pointer hosted in `libusd_tf.so`.
- **Tradeoff**: Adds only `~90 KB` (stripped) to `libusd_tf.so` while removing two shared libraries (`libusd_python.so` and `libusd_boost.so`) from dynamic linker startup across all USD tools and plugins, with zero patches to vendored `nanobind`.

### 2.2 Automatic `TfErrorMark` $\rightarrow$ Python Exception Translation via Vectorcall Wrapping
- **Challenge**: OpenUSD C++ APIs report runtime/coding errors via `TF_RUNTIME_ERROR` / `TF_CODING_ERROR` into thread-local `TfErrorMark`s rather than throwing C++ exceptions. `Boost.Python` checked a `TfErrorMark` inside its central `caller` template after every C++ invocation.
- **Design** ([`pxr/base/tf/pyModule.cpp`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyModule.cpp)): Rather than wrapping every `.def()` lambda in every `wrap*.cpp` file with a `TfErrorMark` (which would bloat binary size and require touching thousands of call sites), `_TfNbWrapModuleFunctions` intercepts `nanobind`'s function (`nb_func`), bound method (`nb_method`), and property (`nb_static_property`) `tp_vectorcall` / descriptor slots once at module initialization. Every Python $\rightarrow$ C++ call automatically establishes a `TfErrorMark` and converts any emitted `TfError`s into Python exceptions via `TfPyConvertTfErrorsToPythonException`.
- **Tradeoff**: Zero boilerplate in `wrap*.cpp` files and zero C++ template bloat, at the cost of a small reliance on `nanobind`'s internal type objects (`nb_func`, `nb_method`).

### 2.3 Object Identity & Smart Pointer Casters (`TfRefPtr`, `TfWeakPtr`, `SdfHandle`)
- **Challenge**: OpenUSD guarantees object identity for `TfRefPtr` and `TfWeakPtr` (`stage1 is stage2` in Python when pointing to the same `UsdStage`), tracks expired weak pointers to raise `Tf.ExpiredObjectError` on method access, and supports polymorphic C++ $\rightarrow$ Python downcasting via `TfType`.
- **Design** ([`pxr/base/tf/pyIdentity.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyIdentity.h), [`pxr/base/tf/pyPtrHelpers.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyPtrHelpers.h), [`pxr/base/tf/pyNanobindBridge.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyNanobindBridge.h), [`pxr/usd/sdf/pySpec.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/usd/sdf/pySpec.h)):
  - Custom `nanobind::detail::type_caster` specializations for `TfRefPtr<T>`, `TfWeakPtr<T>` (in `tf/pyNanobindBridge.h`), and `SdfHandle<SpecType>` (in `sdf/pySpec.h`) consult `Tf_PyIdentityHelper` before constructing a new Python wrapper and attach weak-reference cleanup callbacks when a new wrapper is created.
  - Dereferencing a `TfWeakPtr<T>` or `SdfHandle<T>` inside `from_python` validates liveness and raises an expired-object error if the underlying C++ object has been destroyed.

### 2.4 C++ / Python Polymorphism (`TfPyPolymorphic<T>`)
- **Design** ([`pxr/base/tf/pyPolymorphic.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyPolymorphic.h)): Migrated `TfPyPolymorphic<T>` from `pxr_boost::python::wrapper<T>` to `nanobind`'s trampoline mechanism while preserving `GetOverride()` / `GetPureVirtualOverride()` and `Tf_PySetPythonIdentity` so Python subclasses of `SdfFileFormat`, `ArResolver`, `TfNotice`, and `UsdValidationValidator` preserve their Python instance identity when passed through C++.

### 2.5 `TfPyWrapEnum` and `VtValue` Type-Erased Registry
- **`TfPyWrapEnum`** ([`pxr/base/tf/pyEnum.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyEnum.h), [`pxr/base/tf/pyEnum.cpp`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyEnum.cpp)): Preserved OpenUSD's `Tf_PyEnumWrapper` hierarchy (`Tf.Tf_PyEnumWrapper`, bitwise ops, `displayName`, `fullName`, `GetValueFromName`) and intercepted `nanobind::detail::enum_from_python` / `enum_from_cpp` in `libusd_tf.so` to consult `Tf_PyEnumRegistry` before falling back to `nanobind`'s native enum lookup.
- **`VtValue`, `VtDictionary`, & `VtArray<T>`** ([`pxr/base/vt/valueFromPython.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/vt/valueFromPython.h), [`pxr/base/vt/valueFromPython.cpp`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/vt/valueFromPython.cpp)): Replaced `Boost.Python` LValue/RValue extractors in `Vt_ValueFromPythonRegistry` with `nanobind::detail::nb_type_get` exact-type lookups and `nanobind::detail::make_caster<T>` implicit conversions, and localized `type_caster<VtArray<T>>`, `type_caster<VtValue>`, `type_caster<VtValueRef>`, and `type_caster<VtDictionary>` in `vt/valueFromPython.h` so C++ functions taking or returning `VtValue` / `VtDictionary` / `VtArray<T>` seamlessly box and unbox Python objects without leaking `<nanobind/nanobind.h>` into C++ headers or upward-depending from `tf`.

### 2.6 Modularized Type Casters for `Tf`, `Gf`, `Usd`, and `Ar`
- **`std::function<Return(Args...)>` Caster** ([`pxr/base/tf/pyFunction.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/tf/pyFunction.h)): Localized in `tf/pyFunction.h` using `TfPyFunctionFromPython` to support weak-referenced bound Python methods, lambdas, and `None` $\rightarrow$ empty `std::function`.
- **`GfHalf` Caster & `Gf` Sequence Conversions** ([`pxr/base/gf/pyBufferUtils.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/gf/pyBufferUtils.h), [`pxr/base/gf/wrapVec.template.cpp`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/base/gf/wrapVec.template.cpp)): Localized `type_caster<GfHalf>` in `gf/pyBufferUtils.h` and registered implicit sequence constructors (`nanobind::init_implicit<nanobind::sequence>()`) on `GfVec*`, `GfMatrix*`, and `GfRange*` types.
- **`UsdObject`, `UsdProperty`, and `UsdPrimRange` Casters** ([`pxr/usd/usd/pyConversions.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/usd/usd/pyConversions.h)): Custom `type_caster` specializations isolated in `usd/pyConversions.h` (rather than public C++ headers) automatically downcast `UsdObject` and `UsdProperty` return values to their most-derived Python wrapper (`Usd.Prim`, `Usd.Attribute`, `Usd.Relationship`, `Usd.Stage`).
- **`ArResolverContext` & `ArAsset` Casters** ([`pxr/usd/ar/pyResolverContext.h`](file:///usr/local/google/home/vicini/code/OpenUSD/pxr/usd/ar/pyResolverContext.h)): Localized in `ar/pyResolverContext.h`; accepts `Ar.ResolverContext` in both pass 1 and pass 2, and accepts registered individual context objects (e.g., `Ar.DefaultResolverContext`) or Python lists/tuples of context objects during pass 2 (`convert=true`), preventing greedy overload hijacking when `Usd.Stage.Open(layer, ...)` is called.

---

## 3. Benchmark Results

**Test Environment**:
- **Hardware / OS**: Linux x86_64, 24 CPU cores
- **Compiler**: Clang 19.1.7 (`-O3 -DNDEBUG`, `CMAKE_BUILD_TYPE=Release`)
- **Python**: Python 3.13.15
- **Benchmark Suite**: [`nanobind_benchmarks/benchmark_usd.py`](file:///usr/local/google/home/vicini/code/OpenUSD/nanobind_benchmarks/benchmark_usd.py) (7 timed repetitions after 2 warmup runs, GC disabled during timed batches)

### 3.1 Runtime Performance Before vs. After `nanobind`

| Benchmark Test Case | Boost.Python Median (ms) | nanobind Median (ms) | Boost.Python Ops/sec | nanobind Ops/sec | Speedup |
| :--- | ---: | ---: | ---: | ---: | ---: |
| `Gf.Vec3d` math (50k ops) | 305.77 | 57.67 | 163,522 | 866,942 | **5.30x** |
| `Gf.Matrix4d` transform & inverse (30k ops) | 91.23 | 20.45 | 328,849 | 1,467,041 | **4.46x** |
| `Gf.Range3d` & `Gf.Quatd` (30k ops) | 81.40 | 23.07 | 368,546 | 1,300,322 | **3.53x** |
| `VtArray` construction & indexing (2k arrays) | 46.65 | 9.12 | 42,871 | 219,219 | **5.11x** |
| `Sdf.Path` manipulation (40k ops) | 79.88 | 41.21 | 500,775 | 970,715 | **1.94x** |
| `Sdf.Layer` `PrimSpec` authoring (2k prims) | 26.07 | 14.89 | 76,719 | 134,352 | **1.75x** |
| `Usd.Stage` define & traverse (2k prims) | 123.21 | 94.21 | 16,232 | 21,228 | **1.31x** |
| `Usd.Attribute` time-samples set/get (5k samples) | 40.52 | 33.81 | 123,393 | 147,891 | **1.20x** |
| `UsdGeom` `XformCache` & `BBoxCache` (1k prims) | 40.42 | 38.89 | 24,741 | 25,716 | **1.04x** |
| `UsdShade` `Material` & `Shader` binding (500 mats) | 159.30 | 145.89 | 3,139 | 3,427 | **1.09x** |
| `UsdLux` & `UsdPhysics` schema authoring (800 prims) | 225.52 | 199.17 | 3,547 | 4,017 | **1.13x** |
| **Geometric Mean** | | | | | **2.07x** |

> **Analysis**: Fine-grained bindings where Python $\leftrightarrow$ C++ dispatch and object allocation dominate (`Gf.Vec3d`, `VtArray`, `Gf.Matrix4d`, `Gf.Range3d`/`Gf.Quatd`, `Sdf.Path`) see **1.94x to 5.30x** speedups. Higher-level `Usd.Stage`, `UsdGeom`, `UsdShade`, and `UsdPhysics` benchmarks—where most time is spent inside C++ composition, layer authoring, and `TfToken` / `SdfLayer` internals—still achieve **1.04x to 1.31x** end-to-end speedups.

---

### 3.2 Binary Sizes Before vs. After `nanobind`

Measured across all **41 common `pxr/*/_*.so` Python extension modules** (both unstripped and stripped with `strip --strip-unneeded`):

| Module / Category | Boost.Python (Unstripped) | nanobind (Unstripped) | Boost.Python (Stripped) | nanobind (Stripped) | Ratio (BP / NB) | Change |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| **All 41 `_*.so` modules (Total)** | **108.77 MB** | **46.03 MB** | **71.11 MB** | **30.88 MB** | **2.36x** | **-57.7%** |
| `_vt.so` | 24.02 MB | 13.32 MB | 16.26 MB | 9.41 MB | **1.80x** | **-44.5%** |
| `_sdf.so` | 17.80 MB | 7.49 MB | 11.45 MB | 4.98 MB | **2.37x** | **-57.9%** |
| `_gf.so` | 12.11 MB | 4.59 MB | 7.92 MB | 2.98 MB | **2.64x** | **-62.1%** |
| `_usd.so` | 8.64 MB | 3.17 MB | 5.57 MB | 2.08 MB | **2.73x** | **-63.3%** |
| `_usdPhysics.so` | 7.89 MB | 1.43 MB | 5.10 MB | 0.93 MB | **5.51x** | **-81.9%** |
| `_usdGeom.so` | 5.23 MB | 2.30 MB | 3.45 MB | 1.48 MB | **2.27x** | **-56.0%** |
| `_tf.so` | 3.15 MB | 1.21 MB | 2.09 MB | 0.82 MB | **2.60x** | **-61.5%** |
| `_usdShade.so` | 2.96 MB | 1.03 MB | 1.89 MB | 0.66 MB | **2.87x** | **-65.1%** |
| `_pcp.so` | 2.51 MB | 0.88 MB | 1.63 MB | 0.59 MB | **2.84x** | **-64.8%** |
| `_usdLux.so` | 2.46 MB | 1.05 MB | 1.56 MB | 0.66 MB | **2.35x** | **-57.5%** |
| `_usdSkel.so` | 2.26 MB | 0.91 MB | 1.46 MB | 0.61 MB | **2.48x** | **-59.7%** |
| `_sdr.so` | 2.04 MB | 0.87 MB | 1.34 MB | 0.55 MB | **2.36x** | **-57.6%** |
| `libusd_python.so` + `libusd_boost.so` | 0.41 MB | *Removed* | 0.29 MB | *Removed* | — | **-100.0%** |
| `libusd_tf.so` (hosts `nanobind` runtime) | 1.77 MB | 1.87 MB | 1.38 MB | 1.47 MB | 0.95x | +5.6% |

---

### 3.3 Build Times Before vs. After `nanobind`

Measured from a clean state across all **545 Python wrapper translation units** (`wrap*.cpp.o` and `module.cpp.o`) and the binding runtime objects (`27` `boost::python` TUs on `dev` vs. `12` `nanobind/src` TUs on `nanobind`) using `clang++-19 -O3 -DNDEBUG` and `ninja -j 24`:

| Build Metric / Target Group | Boost.Python (`dev`) | nanobind | Speedup / Change |
| :--- | ---: | ---: | ---: |
| **Wall-clock time (`ninja -j 24`, all Python bindings)** | **154.40 s** | **135.01 s** | **1.14x (-12.6%)** |
| **Total CPU time (`user + sys`, all Python bindings)** | **3,109.06 s** | **3,017.56 s** | **1.03x (-2.9%)** |
| **Binding runtime (`libusd_python` vs. `nanobind/src`)** | 36.1 s (27 files) | 19.5 s (12 files) | **1.85x (-46.0%)** |
| `_usdPhysics` wrapper TUs (21 files) | 179.8 s | 149.1 s | **1.21x (-17.1%)** |
| `_vt` wrapper TUs (16 files) | 266.0 s | 223.3 s | **1.19x (-16.0%)** |
| `_usd` wrapper TUs (46 files) | 322.1 s | 300.8 s | **1.07x (-6.6%)** |
| `_sdf` wrapper TUs (37 files) | 261.9 s | 244.9 s | **1.07x (-6.5%)** |
| `_usdLux` wrapper TUs (23 files) | 177.0 s | 165.4 s | **1.07x (-6.5%)** |
| `_usdGeom` wrapper TUs (40 files) | 314.9 s | 296.2 s | **1.06x (-5.9%)** |
| `_pcp` wrapper TUs (20 files) | 115.6 s | 109.4 s | **1.06x (-5.4%)** |
| `_usdVol` wrapper TUs (22 files) | 159.2 s | 151.0 s | **1.05x (-5.1%)** |
| `_usdShade` wrapper TUs (16 files) | 111.6 s | 110.3 s | **1.01x (-1.1%)** |
| `_usdSkel` wrapper TUs (18 files) | 121.5 s | 123.0 s | 0.99x (+1.2%) |
| `_gf` wrapper TUs (56 files) | 196.1 s | 216.1 s | 0.91x (+10.2%) |
| `_tf` wrapper TUs (37 files) | 96.5 s | 114.0 s | 0.85x (+18.2%) |
