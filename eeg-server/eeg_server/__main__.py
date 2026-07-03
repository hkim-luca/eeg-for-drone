"""Entry point: ``python -m eeg_server`` starts the TCP link and the dashboard."""

from __future__ import annotations

import asyncio
import logging

from . import config
from .dashboard import start_dashboard
from .state import ServerState
from .tcp_server import EegTcpServer


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(name)s %(levelname)s %(message)s")

    state = ServerState()
    http_server = start_dashboard(state)
    tcp_server = EegTcpServer(state)

    print(f"EEG dummy server ready - DroneSim link tcp://{config.TCP_HOST}:{config.TCP_PORT}, "
          f"dashboard http://{config.HTTP_HOST}:{config.HTTP_PORT}/ (Ctrl+C to stop)")
    try:
        asyncio.run(tcp_server.serve_forever())
    except KeyboardInterrupt:
        pass
    finally:
        http_server.shutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
