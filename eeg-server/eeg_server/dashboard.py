"""Dashboard HTTP endpoint: static page plus a JSON state snapshot.

Runs on a daemon thread with the standard-library HTTP server; the page
(``static/index.html``) polls ``/api/state`` a few times per second, which is
plenty for evaluation readouts and keeps the server dependency-free.
"""

from __future__ import annotations

import json
import logging
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Final

from . import config
from .state import ServerState

_LOGGER: Final[logging.Logger] = logging.getLogger("eeg_server.http")

_STATIC_DIR: Final[Path] = Path(__file__).parent / "static"

_CONTENT_TYPES: Final[dict[str, str]] = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
}


class _DashboardHandler(BaseHTTPRequestHandler):
    """Serves the dashboard page and the state snapshot."""

    #: Injected by :func:`start_dashboard` before the server starts.
    state: ServerState

    def do_GET(self) -> None:  # noqa: N802 - fixed name from BaseHTTPRequestHandler
        if self.path == "/api/state":
            body = json.dumps(self.state.snapshot(), separators=(",", ":")).encode("utf-8")
            self._send_bytes(body, "application/json")
            return

        # static files: flat whitelist under static/, no path components
        name = "index.html" if self.path == "/" else Path(self.path).name
        target = _STATIC_DIR / name
        content_type = _CONTENT_TYPES.get(target.suffix)
        if content_type is not None and target.is_file():
            self._send_bytes(target.read_bytes(), content_type)
        else:
            self.send_error(404)

    def _send_bytes(self, body: bytes, content_type: str) -> None:
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format: str, *args: object) -> None:  # noqa: A002 - fixed name
        # polling every 500 ms floods the default request log; keep it quiet
        pass


def start_dashboard(state: ServerState) -> ThreadingHTTPServer:
    """Starts the dashboard HTTP server on a daemon thread and returns it."""
    handler = type("BoundDashboardHandler", (_DashboardHandler,), {"state": state})
    server = ThreadingHTTPServer((config.HTTP_HOST, config.HTTP_PORT), handler)
    thread = threading.Thread(target=server.serve_forever, name="dashboard-http", daemon=True)
    thread.start()
    _LOGGER.info("dashboard on http://%s:%d/", config.HTTP_HOST, config.HTTP_PORT)
    return server
