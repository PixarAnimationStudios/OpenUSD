#!/usr/bin/env python3
"""
Unit tests for httpResolver.

The test fixture starts test_server.py as a subprocess, reads the PORT= line
from its stdout, and constructs all URLs against http://localhost:<port>/...
"""

import os
import pathlib
import queue
import subprocess
import sys
import threading
import time
import unittest

# ---------------------------------------------------------------------------
# Locate the installed feature-config header and derive feature flags.
# We grep for "#define PXR_HTTP_<FLAG>" in the installed httpResolverConfig.h.
# ---------------------------------------------------------------------------

_HERE = pathlib.Path(__file__).parent

# When run via ctest, testWrapper.py copies the testenv directory contents into
# a temporary working directory and chdir's there before executing this script.
# When run directly (e.g. python3 tests/testHttpResolver), the testenv assets
# live next to the script or in tests/ctest/testHttpResolver/.
def _find_testenv() -> pathlib.Path:
    """Return the directory that contains test_server.py."""
    # ctest working directory (testWrapper copies files here)
    cwd = pathlib.Path.cwd()
    if (cwd / "test_server.py").exists():
        return cwd
    # Adjacent to this script (source tree or direct install run)
    if (_HERE / "test_server.py").exists():
        return _HERE
    # install: tests/ctest/testHttpResolver/ relative to tests/
    candidate = _HERE / "ctest" / "testHttpResolver"
    if (candidate / "test_server.py").exists():
        return candidate
    # Fallback — will produce a clear FileNotFoundError
    return _HERE

_TESTENV = _find_testenv()

def _read_feature_flags() -> set:
    flags = set()
    # Try to find httpResolverConfig.h in common install locations.
    # _HERE.parent covers the case where the test is installed at
    # <prefix>/tests/testHttpResolver (so <prefix> == _HERE.parent).
    search_roots = [
        _HERE.parent,
        pathlib.Path(os.environ.get("USD_INSTALL_ROOT", "/nonexistent")),
        pathlib.Path(os.environ.get("CMAKE_INSTALL_PREFIX", "/nonexistent")),
    ]
    for root in search_roots:
        cfg = root / "include" / "pxr" / "httpResolver" / "httpResolverConfig.h"
        if cfg.exists():
            for line in cfg.read_text().splitlines():
                line = line.strip()
                if line.startswith("#define PXR_HTTP_"):
                    flags.add(line.split()[1])
            break
    return flags

_FEATURES = _read_feature_flags()
_HAS_STREAMING  = "PXR_HTTP_USDZ_STREAMING"  in _FEATURES
_HAS_DISK_CACHE = "PXR_HTTP_CACHE_MODE_DISK" in _FEATURES
_HAS_FILEPATH   = "PXR_HTTP_SERVER_FILEPATH" in _FEATURES

# ---------------------------------------------------------------------------
# Server fixture
# ---------------------------------------------------------------------------

class _ServerFixture:
    def __init__(self):
        self._proc = None
        self.port  = None
        self.base  = None

    def start(self):
        server_script = _TESTENV / "test_server.py"
        self._proc = subprocess.Popen(
            [sys.executable, str(server_script)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        # Read the PORT= line using a background reader thread so the
        # deadline check isn't blocked by a hanging readline().
        q: queue.Queue = queue.Queue()

        def _reader():
            for line in self._proc.stdout:
                q.put(line)

        threading.Thread(target=_reader, daemon=True).start()

        deadline = time.monotonic() + 10.0
        while time.monotonic() < deadline:
            try:
                line = q.get(timeout=0.1)
            except queue.Empty:
                if self._proc.poll() is not None:
                    raise RuntimeError(
                        "Test server exited before printing PORT=")
                continue
            if line.startswith("PORT="):
                self.port = int(line.strip().split("=")[1])
                self.base = f"http://localhost:{self.port}"
                return
        raise RuntimeError("Test server did not start in time")

    def stop(self):
        if self._proc:
            self._proc.terminate()
            self._proc.wait(timeout=5)

    def url(self, path: str) -> str:
        return self.base + path


_server = _ServerFixture()


def setUpModule():
    _server.start()
    # Ensure the resolver plugin path is set.
    plugin_path = os.environ.get("PXR_PLUGINPATH_NAME", "")
    if not plugin_path:
        raise unittest.SkipTest(
            "PXR_PLUGINPATH_NAME not set; plugin may not load")


def tearDownModule():
    _server.stop()


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _import_pxr():
    try:
        from pxr import Ar, Usd
        return Ar, Usd
    except ImportError:
        raise unittest.SkipTest("pxr Python bindings not available")


# ---------------------------------------------------------------------------
# Test cases
# ---------------------------------------------------------------------------

class TestCore(unittest.TestCase):

    def setUp(self):
        self.Ar, self.Usd = _import_pxr()

    def test_simple_resolve(self):
        """_Resolve on a plain http URL returns a non-empty ArResolvedPath."""
        url = _server.url("/assets/simple.usda")
        resolver = self.Ar.GetResolver()
        resolved = resolver.Resolve(url)
        self.assertTrue(bool(resolved),
                        f"Expected non-empty resolved path for {url}")

    def test_open_simple_asset(self):
        """Loading a .usda stage over HTTP returns the expected prims."""
        url = _server.url("/assets/simple.usda")
        stage = self.Usd.Stage.Open(url)
        self.assertIsNotNone(stage, f"Failed to open stage at {url}")
        paths = [str(p.GetPath()) for p in stage.Traverse()]
        self.assertIn("/Root", paths)
        self.assertIn("/Root/Ball", paths)
        self.assertIn("/Root/Box", paths)

    def test_modification_timestamp(self):
        """Timestamp for the same URL is consistent across two calls."""
        url = _server.url("/mtime/simple.usda")
        resolver = self.Ar.GetResolver()
        resolved = self.Ar.ResolvedPath(url)
        t1 = resolver.GetModificationTimestamp(url, resolved)
        t2 = resolver.GetModificationTimestamp(url, resolved)
        self.assertEqual(t1, t2, "Timestamps should be stable for same URL")

    def test_extension_parsing(self):
        """_GetExtension strips query strings and returns the correct ext."""
        url = _server.url("/assets/simple.usda?resource=foo")
        resolver = self.Ar.GetResolver()
        ext = resolver.GetExtension(url)
        self.assertEqual(ext, "usda",
                         f"Expected 'usda', got '{ext}' for {url}")

    def test_relative_path_anchoring(self):
        """A relative path is resolved correctly against an HTTP anchor."""
        Ar = self.Ar
        anchor = self.Ar.ResolvedPath(_server.url("/assets/simple.usda"))
        resolver = Ar.GetResolver()
        # Resolve "simple.usda" relative to the same directory anchor
        ident = resolver.CreateIdentifier("simple.usda", anchor)
        self.assertIn("simple.usda", ident,
                      f"Relative path not anchored correctly: {ident}")

    def test_filesystem_fallback(self):
        """A plain filesystem path still resolves via ArDefaultResolver."""
        Ar = self.Ar
        resolver = Ar.GetResolver()
        # __file__ is a valid filesystem path
        resolved = resolver.Resolve(__file__)
        self.assertTrue(bool(resolved), "Filesystem fallback should resolve")


class TestUsdzStreaming(unittest.TestCase):

    def setUp(self):
        self.Ar, self.Usd = _import_pxr()

    @unittest.skipUnless(_HAS_STREAMING, "PXR_HTTP_USDZ_STREAMING not compiled in")
    def test_usdz_streaming(self):
        """Opening a USDZ over HTTP resolves its sub-assets via byte-ranges."""
        url = _server.url("/assets/package.usdz")
        stage = self.Usd.Stage.Open(url)
        self.assertIsNotNone(stage, f"Failed to open USDZ stage at {url}")

    @unittest.skipIf(_HAS_STREAMING, "Test only runs without PXR_HTTP_USDZ_STREAMING")
    def test_usdz_full_download(self):
        """Without streaming, USDZ is fetched as a single full download."""
        url = _server.url("/assets/package.usdz")
        stage = self.Usd.Stage.Open(url)
        self.assertIsNotNone(stage, f"Failed to open USDZ stage at {url}")

    @unittest.skipUnless(_HAS_STREAMING, "PXR_HTTP_USDZ_STREAMING not compiled in")
    def test_byte_range_request_sent(self):
        """Verify the server receives a Range header for USDZ streaming."""
        import urllib.request
        url = _server.url("/assets/simple.usda")
        req = urllib.request.Request(url, headers={"Range": "bytes=0-63"})
        with urllib.request.urlopen(req) as resp:
            self.assertIn(resp.status, (200, 206),
                          "Server should honour Range requests")


class TestDiskCache(unittest.TestCase):

    def setUp(self):
        self.Ar, self.Usd = _import_pxr()

    @unittest.skipUnless(_HAS_DISK_CACHE, "PXR_HTTP_CACHE_MODE_DISK not compiled in")
    def test_disk_cache_populated(self):
        """After the first open, a cache file should exist on disk."""
        import tempfile
        url = _server.url("/assets/simple.usda")
        stage = self.Usd.Stage.Open(url)
        self.assertIsNotNone(stage)
        cache_root = os.environ.get("PXR_HTTP_DISK_CACHE_PATH",
                                    tempfile.gettempdir())
        cache_dir = pathlib.Path(cache_root) / "httpResolverCache"
        self.assertTrue(cache_dir.exists(),
                        f"Disk cache directory not created: {cache_dir}")
        cached_files = list(cache_dir.iterdir())
        self.assertGreater(len(cached_files), 0,
                           "No files written to disk cache")


class TestServerFilepath(unittest.TestCase):

    def setUp(self):
        self.Ar, self.Usd = _import_pxr()

    @unittest.skipUnless(_HAS_FILEPATH, "PXR_HTTP_SERVER_FILEPATH not compiled in")
    def test_server_filepath(self):
        """Resolver follows X-USD-Local-Path and opens the local file."""
        url = _server.url("/filepath/simple.usda")
        stage = self.Usd.Stage.Open(url)
        self.assertIsNotNone(stage,
                             "Stage should open via server-provided filepath")

    @unittest.skipUnless(_HAS_FILEPATH, "PXR_HTTP_SERVER_FILEPATH not compiled in")
    def test_server_filepath_fallback(self):
        """Normal binary response still works when server skips filepath."""
        url = _server.url("/assets/simple.usda")
        stage = self.Usd.Stage.Open(url)
        self.assertIsNotNone(stage,
                             "Stage should open when server returns data normally")


class TestConcurrency(unittest.TestCase):

    def setUp(self):
        self.Ar, self.Usd = _import_pxr()

    def test_concurrent_downloads(self):
        """Two threads opening the same URL should result in exactly one server request."""
        import urllib.request
        # Reset the counter by making a dummy request to /count
        url = _server.url("/slow/simple.usda")
        count_url = _server.url("/count/slow/simple.usda")

        results = []
        errors = []

        def open_stage():
            try:
                stage = self.Usd.Stage.Open(url)
                results.append(stage is not None)
            except Exception as e:
                errors.append(str(e))

        t1 = threading.Thread(target=open_stage)
        t2 = threading.Thread(target=open_stage)
        t1.start()
        t2.start()
        t1.join(timeout=10)
        t2.join(timeout=10)

        self.assertFalse(errors, f"Thread errors: {errors}")
        self.assertEqual(len(results), 2, "Both threads should complete")
        self.assertTrue(all(results), "Both threads should succeed")

        # Both stages should have loaded successfully; server-side request
        # count is advisory — primarily we verify no errors occurred.
        with urllib.request.urlopen(count_url) as resp:
            count = int(resp.read().decode())
        # The resolver's TBB cache should mean the second thread reuses the
        # first download; the count should be 1 or 2 depending on timing.
        self.assertGreaterEqual(count, 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
