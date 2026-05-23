#!/usr/bin/env python3
"""
Test HTTP server for httpResolver tests.

Endpoints:
  GET  /assets/<file>    - Serve static asset; honours Range: header
  HEAD /assets/<file>    - Return headers only (for mtime tests)
  GET  /mtime/<file>     - Serve asset with a fixed Last-Modified header
  GET  /filepath/<file>  - Return Content-Type: application/x-usd-filepath
                           and X-USD-Local-Path pointing to the on-disk file
                           (only served when PXR_HTTP_SERVER_FILEPATH is compiled in)
  GET  /slow/<file>      - Same as /assets/ but with a configurable delay
                           (used for concurrent-download deduplication tests)

The server prints "PORT=<n>" to stdout on startup so the test harness can
discover the dynamically assigned port.
"""

import email.utils
import http.server
import os
import pathlib
import socketserver
import sys
import threading
import time

ASSETS_DIR = pathlib.Path(__file__).parent / "assets"

# Counts of server-side GET requests per path, used by concurrency tests.
_request_counts: dict[str, int] = {}
_request_counts_lock = threading.Lock()


def _record_request(path: str) -> int:
    with _request_counts_lock:
        _request_counts[path] = _request_counts.get(path, 0) + 1
        return _request_counts[path]


def _get_request_count(path: str) -> int:
    with _request_counts_lock:
        return _request_counts.get(path, 0)


class Handler(http.server.BaseHTTPRequestHandler):

    def log_message(self, fmt, *args):  # suppress default access log noise
        pass

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _asset_path(self, rel: str) -> pathlib.Path:
        """Return the on-disk path for a relative asset name, or None."""
        # Prevent path traversal
        safe = pathlib.Path(rel.lstrip("/"))
        full = (ASSETS_DIR / safe).resolve()
        try:
            full.relative_to(ASSETS_DIR.resolve())
        except ValueError:
            return None
        return full if full.exists() else None

    def _serve_bytes(self, data: bytes, content_type: str,
                     extra_headers: dict | None = None,
                     status: int = 200) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        if extra_headers:
            for k, v in extra_headers.items():
                self.send_header(k, v)
        self.end_headers()
        self.wfile.write(data)

    def _serve_file(self, path: pathlib.Path, extra_headers: dict | None = None,
                    head_only: bool = False) -> None:
        """Serve a file, honouring Range: requests and generating Last-Modified."""
        stat = path.stat()
        mtime = stat.st_mtime
        last_modified = email.utils.formatdate(mtime, usegmt=True)
        size = stat.st_size

        # Parse Range header
        range_header = self.headers.get("Range", "")
        start = 0
        end = size  # exclusive

        if range_header.startswith("bytes="):
            rng = range_header[6:]
            if rng.startswith("-"):
                # suffix range
                suffix = int(rng[1:])
                start = max(0, size - suffix)
            elif "-" in rng:
                parts = rng.split("-", 1)
                start = int(parts[0]) if parts[0] else 0
                end = int(parts[1]) + 1 if parts[1] else size
            end = min(end, size)

        headers = {
            "Last-Modified": last_modified,
            "Accept-Ranges": "bytes",
        }
        if extra_headers:
            headers.update(extra_headers)

        if range_header and start != 0:
            status = 206
            headers["Content-Range"] = f"bytes {start}-{end - 1}/{size}"
        else:
            status = 200

        content_length = end - start
        self.send_response(status)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(content_length))
        for k, v in headers.items():
            self.send_header(k, v)
        self.end_headers()

        if not head_only:
            with open(path, "rb") as f:
                f.seek(start)
                remaining = content_length
                while remaining > 0:
                    chunk = f.read(min(65536, remaining))
                    if not chunk:
                        break
                    self.wfile.write(chunk)
                    remaining -= len(chunk)

    # ------------------------------------------------------------------
    # Request handlers
    # ------------------------------------------------------------------

    def do_HEAD(self):
        parts = self.path.lstrip("/").split("/", 1)
        prefix, rel = parts[0], (parts[1] if len(parts) > 1 else "")

        if prefix == "assets":
            p = self._asset_path(rel)
            if p:
                self._serve_file(p, head_only=True)
            else:
                self.send_response(404)
                self.end_headers()
        else:
            self.send_response(404)
            self.end_headers()

    def do_GET(self):
        parts = self.path.lstrip("/").split("/", 1)
        prefix, rel = parts[0], (parts[1] if len(parts) > 1 else "")

        _record_request(self.path)

        if prefix == "assets":
            p = self._asset_path(rel)
            if p:
                self._serve_file(p)
            else:
                self.send_response(404)
                self.end_headers()

        elif prefix == "mtime":
            # Serve with a pinned Last-Modified so the test can control it.
            p = self._asset_path(rel)
            if p:
                # Fixed epoch time so tests are deterministic.
                fixed_time = email.utils.formatdate(1_000_000.0, usegmt=True)
                self._serve_file(p, extra_headers={"Last-Modified": fixed_time})
            else:
                self.send_response(404)
                self.end_headers()

        elif prefix == "filepath":
            # Return a pointer to the local file instead of its bytes.
            p = self._asset_path(rel)
            if p:
                payload = b""
                self.send_response(200)
                self.send_header("Content-Type", "application/x-usd-filepath")
                self.send_header("X-USD-Local-Path", str(p))
                self.send_header("Content-Length", "0")
                self.end_headers()
            else:
                self.send_response(404)
                self.end_headers()

        elif prefix == "count":
            # Return the request count for a path as plain text.
            count = _get_request_count("/" + rel)
            data = str(count).encode()
            self._serve_bytes(data, "text/plain")

        elif prefix == "slow":
            # Slow endpoint — delay 0.5 s before responding (concurrency test)
            time.sleep(0.5)
            p = self._asset_path(rel)
            if p:
                self._serve_file(p)
            else:
                self.send_response(404)
                self.end_headers()

        else:
            self.send_response(404)
            self.end_headers()


class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main():
    with ThreadedTCPServer(("", 0), Handler) as httpd:
        port = httpd.server_address[1]
        # Print port so the test harness can read it.
        print(f"PORT={port}", flush=True)
        httpd.serve_forever()


if __name__ == "__main__":
    main()
