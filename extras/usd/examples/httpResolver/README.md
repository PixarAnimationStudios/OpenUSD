# httpResolver

An example `ArResolver` plugin that enables OpenUSD to load assets directly
over HTTP or HTTPS without writing them to disk first.

## Overview

`httpResolver` subclasses `ArDefaultResolver` and registers itself for `http://`
and `https://` URI schemes.  Once the plugin is on `PXR_PLUGINPATH_NAME`,
any call to `Usd.Stage.Open("http://...")` or `Ar.GetResolver().Resolve("http://...")`
is dispatched here automatically; plain filesystem paths continue to go through
the default resolver.

### Features

| Feature | CMake option | Default |
|---------|-------------|---------|
| USDZ byte-range streaming | `PXR_HTTP_USDZ_STREAMING` | ON |
| In-memory asset cache | `PXR_HTTP_CACHE_MODE=MEMORY` | default |
| Disk-based asset cache | `PXR_HTTP_CACHE_MODE=DISK` | OFF |
| Server-side file path hints | `PXR_HTTP_SERVER_FILEPATH` | OFF |

### Transport backends

The HTTP transport layer is selected at **compile time**:

| Backend | Platform | CMake value |
|---------|----------|-------------|
| Foundation / NSURLSession | macOS, iOS | `PXR_HTTP_BACKEND=NSURL` (default on Apple) |
| libcurl | Linux, Windows, macOS | `PXR_HTTP_BACKEND=CURL` |

Both backends implement the same private `HttpResolver` member functions
(`_IsUrl`, `_GetExtensionFromUrl`, `_MakeFullyQualifiedURL`,
`_GetModificationTimeFromServer`, `_FetchFile`, `_FetchByteRange`).
The platform-agnostic code in `httpResolver.cpp` calls these functions and
never touches backend-specific APIs directly.

## Building

The plugin is built as part of the standard OpenUSD build and is enabled by
default.  To build with all defaults:

```sh
python3 build_usd.py /path/to/install
```

To disable the plugin entirely:

```sh
python3 build_usd.py /path/to/install --no-http-resolver-example
```

To customise features (pass to `cmake` directly or via `build_usd.py --cmake-args`):

```sh
cmake .. \
  -DPXR_HTTP_BACKEND=CURL               # force CURL on macOS (default: NSURL)
  -DPXR_HTTP_USDZ_STREAMING=OFF         # disable byte-range streaming
  -DPXR_HTTP_CACHE_MODE=DISK            # persist downloads to disk
  -DPXR_HTTP_DISK_CACHE_PATH=/tmp/cache # optional: custom cache directory
  -DPXR_HTTP_SERVER_FILEPATH=ON         # enable server-redirect feature
```

### Dependencies

- **NSURL backend** — no extra dependencies (Foundation.framework is part of the Apple SDK).
- **CURL backend** — requires `libcurl` (found via `find_package(CURL REQUIRED)`).

### Generated header

CMake generates `httpResolverConfig.h` from `httpResolverConfig.h.in` and
installs it to `<prefix>/include/pxr/httpResolver/httpResolverConfig.h`.
Include it (or check the installed header) to test which features are compiled in:

```cpp
#include "httpResolverConfig.h"
#ifdef PXR_HTTP_USDZ_STREAMING
// streaming code path
#endif
```

The Python test suite reads the same header to skip tests for features that
were not compiled in.

## Usage

```python
from pxr import Ar, Usd

# Open a remote stage directly
stage = Usd.Stage.Open("http://example.com/assets/shot.usda")

# Resolve a URL
resolver = Ar.GetResolver()
resolved = resolver.Resolve("http://example.com/assets/shot.usda")
```

Relative paths are resolved against their HTTP anchor:

```python
anchor = Ar.ResolvedPath("http://example.com/assets/shot.usda")
ident  = resolver.CreateIdentifier("char.usda", anchor)
# -> "http://example.com/assets/char.usda"
```

### USDZ streaming

When `PXR_HTTP_USDZ_STREAMING` is compiled in (the default), opening a `.usdz`
over HTTP avoids downloading the entire archive.  Instead:

1. The last 128 bytes of the file are fetched to locate the ZIP
   End-of-Central-Directory (EOCD) record.
2. The Central Directory is fetched as a single byte-range request, and every
   entry's name, offset, and size are stored in `_packageInfoCache`.
3. The first entry in the archive is treated as the USDZ entrypoint.  Its name
   is appended to the URL as a `?resource=` query parameter so that `SdfLayer`
   can open it as a sub-asset (e.g. `http://…/archive.usdz?resource=root.usda`).
4. When a sub-asset is subsequently opened, only its byte range is fetched.

Without `PXR_HTTP_USDZ_STREAMING`, the full archive is downloaded, written to a
temporary file (`ArchMakeTmpFileName`), and opened from disk so that the ZIP
parser can use `GetFileUnsafe()`.

### Asset caching

**Memory cache (default)** — downloaded content (full files and byte ranges) is
stored in the TBB concurrent hash map `_downloadCache`.  The cache key for a
full file is the URL; for a byte range it is `"<url>|<start>-<end>"`.  The cache
lives for the lifetime of the process.

**Disk cache (`PXR_HTTP_CACHE_MODE=DISK`)** — content is written to
`$PXR_HTTP_DISK_CACHE_PATH/httpResolverCache/<url-hash>` before being returned
to the caller.  On subsequent opens the file is served directly from disk.  If
`PXR_HTTP_DISK_CACHE_PATH` is not set the system temp directory is used.

### Server-side file paths (`PXR_HTTP_SERVER_FILEPATH`)

When compiled with `PXR_HTTP_SERVER_FILEPATH`, the resolver recognises a special
server response that redirects it to a local file:

```
HTTP/1.1 200 OK
Content-Type: application/x-usd-filepath
X-USD-Local-Path: /mnt/nfs/assets/shot.usda
Content-Length: 0
```

The resolver opens the local path directly via `ArDefaultResolver`, skipping
any in-memory copy.  This is useful for NFS or shared-storage environments
where the HTTP server knows the canonical on-disk location of the asset.

### Modification timestamps

`_GetModificationTimestamp` issues an HTTP HEAD request (NSURL) or a
`CURLOPT_NOBODY=1` GET (CURL) and reads the `Last-Modified` response header.
The result is returned as an `ArTimestamp` (seconds since epoch).

### Extension detection

`_GetExtension` is overridden to handle streaming sub-asset URLs:

- `http://…/archive.usdz?resource=root.usda` → `"usda"` (inner file extension)
- `http://…/archive.usdz` → `"usdz"`
- `http://…/shot.usda` → `"usda"`

Returning the correct extension is critical: `SdfLayer` uses it to select the
right file-format plugin.  A `.usdz?resource=` URL that incorrectly returns
`"usdz"` would cause the ZIP format plugin to be chosen, which requires a real
on-disk file and fails with an `ArInMemoryAsset`.

## Running the tests

The plugin must be installed and `PXR_PLUGINPATH_NAME` must point to its
`resources` directory.  The CMake / ctest runner sets this automatically:

```sh
# from your build directory
ctest -R testHttpResolver --output-on-failure
```

To run the Python test suite directly:

```sh
export PXR_PLUGINPATH_NAME=/path/to/install/share/usd/examples/plugin/httpResolver/resources
export PYTHONPATH=/path/to/install/lib/python
python3 extras/usd/examples/httpResolver/testenv/testHttpResolver.py
```

The test harness starts a local HTTP server (`test_server.py`) on an ephemeral
port.  The server supports:

| Endpoint | Description |
|----------|-------------|
| `GET /assets/<file>` | Serve a static asset; honours `Range:` header |
| `HEAD /assets/<file>` | Return headers only (used by mtime tests) |
| `GET /mtime/<file>` | Same as `/assets/` but with a fixed `Last-Modified` |
| `GET /filepath/<file>` | Return `Content-Type: application/x-usd-filepath` + `X-USD-Local-Path` |
| `GET /slow/<file>` | 0.5 s delay before responding (concurrency deduplication test) |
| `GET /count/<path>` | Return the number of GET requests received for `/<path>` |

### Test assets

- `assets/simple.usda` — basic USDA file with prims `/Root`, `/Root/Ball`, `/Root/Box`
- `assets/package.usdz` — USDZ archive used for streaming and full-download tests

