#!/usr/bin/env python3
"""Benchmark suite comparing OpenUSD Python bindings (Boost.Python vs nanobind)."""

import argparse
import gc
import json
import os
import statistics
import subprocess
import sys
import time
from typing import Any, Callable


def run_benchmark(
    fn: Callable[[], Any],
    iterations: int,
    repeats: int = 5,
    warmup: int = 1,
) -> dict[str, float]:
    """Run a benchmark callable multiple times and collect timing statistics.

    Args:
        fn: Zero-argument callable executing a single benchmark batch.
        iterations: Number of logical operations performed inside one call to fn.
        repeats: Number of timed repetitions to record.
        warmup: Number of untimed warmup repetitions before timing.

    Returns:
        Dictionary containing min, median, mean, stdev (in seconds) and ops_per_sec.
    """
    for _ in range(warmup):
        fn()

    gc_was_enabled = gc.isenabled()
    gc.disable()
    timings = []
    try:
        for _ in range(repeats):
            gc.collect()
            start = time.perf_counter()
            fn()
            timings.append(time.perf_counter() - start)
    finally:
        if gc_was_enabled:
            gc.enable()

    median_s = statistics.median(timings)
    return {
        "min_s": min(timings),
        "median_s": median_s,
        "mean_s": statistics.mean(timings),
        "stdev_s": statistics.stdev(timings) if len(timings) > 1 else 0.0,
        "iterations": iterations,
        "ops_per_sec": iterations / median_s if median_s > 0 else 0.0,
    }


def bench_gf_vec3d(n: int = 50000) -> Callable[[], None]:
    """Create a benchmark exercising Gf.Vec3d construction and math operations.

    Args:
        n: Number of loop iterations per batch.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Gf

    def _run() -> None:
        v1 = Gf.Vec3d(1.0, 2.0, 3.0)
        v2 = Gf.Vec3d(4.0, 5.0, 6.0)
        acc = Gf.Vec3d(0.0, 0.0, 0.0)
        for i in range(n):
            v = Gf.Vec3d(float(i), float(i + 1), float(i + 2))
            c = Gf.Cross(v1, v2) + v
            d = Gf.Dot(c, v1)
            acc += c * (d * 1e-6)
        _ = acc.GetNormalized()

    return _run


def bench_gf_matrix4d(n: int = 30000) -> Callable[[], None]:
    """Create a benchmark exercising Gf.Matrix4d transforms and multiplication.

    Args:
        n: Number of loop iterations per batch.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Gf

    def _run() -> None:
        m1 = Gf.Matrix4d().SetTranslate(Gf.Vec3d(1.0, 2.0, 3.0))
        m2 = Gf.Matrix4d().SetScale(Gf.Vec3d(2.0, 2.0, 2.0))
        pt = Gf.Vec3d(0.5, 1.5, -0.5)
        acc = Gf.Matrix4d(1.0)
        for _ in range(n):
            m = m1 * m2
            pt = m.Transform(pt) * 0.5
            acc = acc * m.GetInverse()

    return _run


def bench_gf_range_quat(n: int = 30000) -> Callable[[], None]:
    """Create a benchmark exercising Gf.Range3d and Gf.Quatd operations.

    Args:
        n: Number of loop iterations per batch.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Gf

    def _run() -> None:
        rng = Gf.Range3d(Gf.Vec3d(-1.0, -1.0, -1.0), Gf.Vec3d(1.0, 1.0, 1.0))
        q1 = Gf.Quatd(1.0, 0.1, 0.2, 0.3).GetNormalized()
        q2 = Gf.Quatd(1.0, -0.2, 0.1, 0.4).GetNormalized()
        q = q1
        for i in range(n):
            q = (q * q2).GetNormalized()
            p = q.Transform(Gf.Vec3d(float(i & 7), 1.0, -1.0))
            rng.UnionWith(p)
            _ = rng.GetMidpoint()

    return _run


def bench_vt_arrays(n: int = 2000) -> Callable[[], None]:
    """Create a benchmark exercising VtArray construction, indexing, and slicing.

    Args:
        n: Number of array construction/access cycles per batch.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Gf, Vt

    pts_src = [Gf.Vec3f(float(i), float(i + 1), float(i + 2)) for i in range(128)]
    idx_src = list(range(256))

    def _run() -> None:
        for _ in range(n):
            pts = Vt.Vec3fArray(pts_src)
            indices = Vt.IntArray(idx_src)
            _ = len(pts) + len(indices)
            _ = pts[0]
            _ = pts[-1]
            _ = indices[10]

    return _run


def bench_sdf_path(n: int = 40000) -> Callable[[], None]:
    """Create a benchmark exercising Sdf.Path creation and manipulation.

    Args:
        n: Number of path operations per batch.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Sdf

    def _run() -> None:
        root = Sdf.Path("/World/Geometry/Characters")
        old_prefix = Sdf.Path("/World/Geometry")
        new_prefix = Sdf.Path("/Scene/Assets")
        for i in range(n):
            child = root.AppendChild(f"Char_{i & 63}")
            prop = child.AppendProperty("xformOp:translate")
            parent = prop.GetParentPath()
            retargeted = parent.ReplacePrefix(old_prefix, new_prefix)
            _ = retargeted.name

    return _run


def bench_sdf_layer_authoring(n: int = 2000) -> Callable[[], None]:
    """Create a benchmark exercising low-level SdfLayer and SdfPrimSpec authoring.

    Args:
        n: Number of prim specs to create and populate per batch.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Sdf

    def _run() -> None:
        layer = Sdf.Layer.CreateAnonymous(".usda")
        with Sdf.ChangeBlock():
            for i in range(n):
                prim_spec = Sdf.CreatePrimInLayer(layer, Sdf.Path(f"/World/Prim_{i}"))
                prim_spec.specifier = Sdf.SpecifierDef
                prim_spec.typeName = "Xform"
                attr_spec = Sdf.AttributeSpec(
                    prim_spec, "weight", Sdf.ValueTypeNames.Double
                )
                attr_spec.default = float(i) * 0.5

    return _run


def bench_usd_stage_populate_and_traverse(num_prims: int = 2000) -> Callable[[], None]:
    """Create a benchmark exercising Usd.Stage prim definition and traversal.

    Args:
        num_prims: Number of prims to define and traverse in the stage.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Usd, UsdGeom

    def _run() -> None:
        stage = Usd.Stage.CreateInMemory()
        root = UsdGeom.Xform.Define(stage, "/World")
        for i in range(num_prims):
            prim = stage.DefinePrim(f"/World/Group_{i >> 5}/Prim_{i}", "Xform")
            prim.SetMetadata("hidden", (i & 1) == 0)

        count = 0
        for prim in stage.Traverse():
            _ = prim.GetPath()
            _ = prim.GetName()
            _ = prim.GetTypeName()
            if prim.IsA(UsdGeom.Xformable):
                count += 1
        _ = root.GetPrim().GetChildren()

    return _run


def bench_usd_attributes_and_timesamples(
    num_prims: int = 100, num_frames: int = 50
) -> Callable[[], None]:
    """Create a benchmark exercising UsdAttribute authoring and time-sample queries.

    Args:
        num_prims: Number of prims with animated attributes.
        num_frames: Number of time samples per attribute.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Gf, Sdf, Usd

    stage = Usd.Stage.CreateInMemory()
    attrs = []
    for i in range(num_prims):
        prim = stage.DefinePrim(f"/World/Anim_{i}", "Xform")
        attr = prim.CreateAttribute("custom:offset", Sdf.ValueTypeNames.Double3)
        attrs.append(attr)

    def _run() -> None:
        _ = stage
        for frame in range(num_frames):
            tc = Usd.TimeCode(float(frame))
            val = Gf.Vec3d(float(frame), float(frame * 2), float(frame * 3))
            for attr in attrs:
                attr.Set(val, tc)

        for frame in range(num_frames):
            tc = Usd.TimeCode(float(frame))
            for attr in attrs:
                _ = attr.Get(tc)

    return _run


def bench_usdgeom_xform_and_bbox(num_prims: int = 1000) -> Callable[[], None]:
    """Create a benchmark exercising UsdGeomXformable, XformCache, and BBoxCache.

    Args:
        num_prims: Number of Xform/Cube prims to author and evaluate.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Gf, Usd, UsdGeom

    stage = Usd.Stage.CreateInMemory()
    prims = []
    for i in range(num_prims):
        if (i & 15) == 0:
            parent = UsdGeom.Xform.Define(stage, f"/World/Parent_{i >> 4}")
            parent.AddTranslateOp().Set(Gf.Vec3d(float(i), 0.0, 0.0))
        cube = UsdGeom.Cube.Define(stage, f"/World/Parent_{i >> 4}/Cube_{i}")
        cube.GetSizeAttr().Set(2.0)
        cube.AddTranslateOp().Set(Gf.Vec3d(0.0, float(i & 15), 0.0))
        cube.AddRotateXYZOp().Set(Gf.Vec3f(15.0, 30.0, 45.0))
        cube.AddScaleOp().Set(Gf.Vec3f(1.0, 2.0, 1.0))
        prims.append(cube.GetPrim())

    def _run() -> None:
        _ = stage
        xform_cache = UsdGeom.XformCache(Usd.TimeCode.Default())
        bbox_cache = UsdGeom.BBoxCache(
            Usd.TimeCode.Default(), [UsdGeom.Tokens.default_]
        )
        for prim in prims:
            _ = xform_cache.GetLocalToWorldTransform(prim)
            _ = bbox_cache.ComputeWorldBound(prim)

    return _run


def bench_usdshade_authoring(num_materials: int = 500) -> Callable[[], None]:
    """Create a benchmark exercising UsdShade material and shader graph authoring.

    Args:
        num_materials: Number of material networks and bindings to author.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Gf, Sdf, Usd, UsdGeom, UsdShade

    def _run() -> None:
        stage = Usd.Stage.CreateInMemory()
        for i in range(num_materials):
            mat_path = f"/World/Looks/Mat_{i}"
            mat = UsdShade.Material.Define(stage, mat_path)
            shader = UsdShade.Shader.Define(stage, f"{mat_path}/PreviewSurface")
            shader.CreateIdAttr("UsdPreviewSurface")
            shader.CreateInput("diffuseColor", Sdf.ValueTypeNames.Color3f).Set(
                Gf.Vec3f(0.8, 0.2, 0.1)
            )
            shader.CreateInput("roughness", Sdf.ValueTypeNames.Float).Set(0.4)
            surf_out = shader.CreateOutput("surface", Sdf.ValueTypeNames.Token)
            mat.CreateSurfaceOutput().ConnectToSource(surf_out)

            mesh = UsdGeom.Mesh.Define(stage, f"/World/Geoms/Mesh_{i}")
            UsdShade.MaterialBindingAPI.Apply(mesh.GetPrim()).Bind(mat)
            _ = UsdShade.MaterialBindingAPI(mesh.GetPrim()).GetDirectBinding().GetMaterial()

    return _run


def bench_usdlux_and_physics_schemas(num_prims: int = 800) -> Callable[[], None]:
    """Create a benchmark exercising UsdLux and UsdPhysics schema classes.

    Args:
        num_prims: Number of prims to author and query with UsdLux/UsdPhysics APIs.

    Returns:
        Zero-argument callable for the benchmark runner.
    """
    from pxr import Gf, Usd, UsdGeom, UsdLux, UsdPhysics

    def _run() -> None:
        stage = Usd.Stage.CreateInMemory()
        for i in range(num_prims):
            light = UsdLux.SphereLight.Define(stage, f"/World/Lights/Light_{i}")
            light.GetIntensityAttr().Set(100.0 + float(i))
            light.GetColorAttr().Set(Gf.Vec3f(1.0, 0.9, 0.8))
            light.GetRadiusAttr().Set(0.5)

            xform = UsdGeom.Cube.Define(stage, f"/World/Bodies/Body_{i}")
            prim = xform.GetPrim()
            rb = UsdPhysics.RigidBodyAPI.Apply(prim)
            rb.CreateVelocityAttr().Set(Gf.Vec3f(0.0, -9.8, 0.0))
            UsdPhysics.CollisionAPI.Apply(prim)
            mass_api = UsdPhysics.MassAPI.Apply(prim)
            mass_api.CreateMassAttr().Set(5.0)
            _ = rb.GetVelocityAttr().Get()
            _ = mass_api.GetMassAttr().Get()

    return _run


def detect_binding_backend() -> str:
    """Detect whether the imported pxr bindings use nanobind or Boost.Python.

    Returns:
        String identifier of the binding backend ('nanobind' or 'Boost.Python').
    """
    from pxr import Gf

    vec_type_repr = repr(type(Gf.Vec3d))
    if "nanobind" in vec_type_repr or hasattr(Gf.Vec3d, "__nb_signature__"):
        return "nanobind"
    if "Boost.Python" in vec_type_repr:
        return "Boost.Python"
    return f"unknown ({vec_type_repr})"


def run_all_benchmarks(repeats: int = 5, warmup: int = 1) -> dict[str, Any]:
    """Execute the full USD benchmark suite in the current process.

    Args:
        repeats: Number of timed repetitions per benchmark.
        warmup: Number of warmup runs per benchmark.

    Returns:
        Dictionary containing environment metadata and per-benchmark statistics.
    """
    benchmarks: list[tuple[str, int, Callable[[], None]]] = [
        ("Gf.Vec3d math (50k ops)", 50000, bench_gf_vec3d(50000)),
        ("Gf.Matrix4d transform & inverse (30k ops)", 30000, bench_gf_matrix4d(30000)),
        ("Gf.Range3d & Gf.Quatd (30k ops)", 30000, bench_gf_range_quat(30000)),
        ("VtArray construction & indexing (2k arrays)", 2000, bench_vt_arrays(2000)),
        ("Sdf.Path manipulation (40k ops)", 40000, bench_sdf_path(40000)),
        ("Sdf.Layer PrimSpec authoring (2k prims)", 2000, bench_sdf_layer_authoring(2000)),
        (
            "Usd.Stage define & traverse (2k prims)",
            2000,
            bench_usd_stage_populate_and_traverse(2000),
        ),
        (
            "Usd.Attribute time-samples set/get (5k samples)",
            5000,
            bench_usd_attributes_and_timesamples(100, 50),
        ),
        (
            "UsdGeom XformCache & BBoxCache (1k prims)",
            1000,
            bench_usdgeom_xform_and_bbox(1000),
        ),
        (
            "UsdShade Material & Shader binding (500 mats)",
            500,
            bench_usdshade_authoring(500),
        ),
        (
            "UsdLux & UsdPhysics schema authoring (800 prims)",
            800,
            bench_usdlux_and_physics_schemas(800),
        ),
    ]

    backend = detect_binding_backend()
    results: dict[str, Any] = {
        "backend": backend,
        "python_version": sys.version.split()[0],
        "benchmarks": {},
    }

    print(f"Running OpenUSD benchmarks [{backend}] (repeats={repeats}, warmup={warmup})...")
    print("-" * 80)
    print(f"{'Benchmark':<48} | {'Median (ms)':>12} | {'Min (ms)':>10} | {'Ops/sec':>12}")
    print("-" * 80)

    for name, iterations, fn in benchmarks:
        stats = run_benchmark(fn, iterations=iterations, repeats=repeats, warmup=warmup)
        results["benchmarks"][name] = stats
        print(
            f"{name:<48} | {stats['median_s'] * 1000.0:>12.2f} | "
            f"{stats['min_s'] * 1000.0:>10.2f} | {stats['ops_per_sec']:>12,.0f}"
        )

    print("-" * 80)
    return results


def run_in_install_env(install_prefix: str, repeats: int, warmup: int) -> dict[str, Any]:
    """Run this benchmark script in a subprocess configured for a given USD install prefix.

    Args:
        install_prefix: Root installation directory of OpenUSD (containing lib/ and lib/python).
        repeats: Number of timed repetitions per benchmark.
        warmup: Number of warmup runs per benchmark.

    Returns:
        Parsed JSON benchmark results from the subprocess.
    """
    py_ver = f"python{sys.version_info.major}.{sys.version_info.minor}"
    candidates = [
        os.path.join(install_prefix, "lib", "python"),
        os.path.join(install_prefix, "lib", py_ver, "site-packages"),
    ]
    pythonpath_entries = [p for p in candidates if os.path.isdir(p)]
    if not pythonpath_entries:
        pythonpath_entries = candidates

    env = os.environ.copy()
    env["PYTHONPATH"] = os.pathsep.join(pythonpath_entries)
    env["LD_LIBRARY_PATH"] = os.path.join(install_prefix, "lib") + (
        os.pathsep + env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else ""
    )

    cmd = [
        sys.executable,
        os.path.abspath(__file__),
        "--repeats",
        str(repeats),
        "--warmup",
        str(warmup),
        "--json-stdout",
    ]
    proc = subprocess.run(cmd, env=env, check=True, capture_output=True, text=True)
    for line in reversed(proc.stdout.splitlines()):
        if line.startswith("{") and line.endswith("}"):
            return json.loads(line)
    raise RuntimeError(f"Failed to parse benchmark JSON output:\n{proc.stdout}\n{proc.stderr}")


def print_comparison_table(boost_res: dict[str, Any], nb_res: dict[str, Any]) -> str:
    """Format and print a Markdown table comparing Boost.Python and nanobind timings.

    Args:
        boost_res: Benchmark result dictionary for the Boost.Python build.
        nb_res: Benchmark result dictionary for the nanobind build.

    Returns:
        Formatted Markdown table string.
    """
    lines = [
        "| Benchmark | Boost.Python Median (ms) | nanobind Median (ms) | Speedup |",
        "| :--- | ---: | ---: | ---: |",
    ]
    speedups = []
    for name, b_stats in boost_res["benchmarks"].items():
        n_stats = nb_res["benchmarks"][name]
        b_ms = b_stats["median_s"] * 1000.0
        n_ms = n_stats["median_s"] * 1000.0
        speedup = b_ms / n_ms if n_ms > 0 else 0.0
        speedups.append(speedup)
        lines.append(f"| {name} | {b_ms:.2f} | {n_ms:.2f} | **{speedup:.2f}x** |")

    geo_mean = statistics.geometric_mean(speedups) if speedups else 0.0
    lines.append(f"| **Geometric Mean** | | | **{geo_mean:.2f}x** |")
    table = "\n".join(lines)
    print("\n" + table + "\n")
    return table


def main() -> None:
    """Parse CLI arguments and run benchmarks or compare two OpenUSD installations.

    Args:
        None.

    Returns:
        None.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repeats", type=int, default=5, help="Number of timed repetitions.")
    parser.add_argument("--warmup", type=int, default=1, help="Number of warmup iterations.")
    parser.add_argument("--json-out", type=str, default="", help="Path to write JSON results.")
    parser.add_argument(
        "--json-stdout",
        action="store_true",
        help="Emit single-line JSON result at the end of stdout.",
    )
    parser.add_argument(
        "--boost-prefix",
        type=str,
        default="",
        help="OpenUSD install prefix built with Boost.Python for side-by-side comparison.",
    )
    parser.add_argument(
        "--nanobind-prefix",
        type=str,
        default="",
        help="OpenUSD install prefix built with nanobind for side-by-side comparison.",
    )
    args = parser.parse_args()

    if args.boost_prefix and args.nanobind_prefix:
        print(f"Benchmarking Boost.Python install at: {args.boost_prefix}")
        boost_res = run_in_install_env(args.boost_prefix, args.repeats, args.warmup)
        print(f"Benchmarking nanobind install at: {args.nanobind_prefix}")
        nb_res = run_in_install_env(args.nanobind_prefix, args.repeats, args.warmup)
        table = print_comparison_table(boost_res, nb_res)
        if args.json_out:
            with open(args.json_out, "w", encoding="utf-8") as f:
                json.dump(
                    {"boost_python": boost_res, "nanobind": nb_res, "markdown_table": table},
                    f,
                    indent=2,
                )
        return

    res = run_all_benchmarks(repeats=args.repeats, warmup=args.warmup)
    if args.json_out:
        with open(args.json_out, "w", encoding="utf-8") as f:
            json.dump(res, f, indent=2)
    if args.json_stdout:
        print(json.dumps(res))


if __name__ == "__main__":
    main()
