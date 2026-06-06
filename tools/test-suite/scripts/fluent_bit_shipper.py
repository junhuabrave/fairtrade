#!/usr/bin/env python3
"""Read fixed-size binary log entries and POST JSON to a logging endpoint asynchronously."""
import argparse
import asyncio
import struct
import json
from pathlib import Path
import aiohttp

# Based on LogEntry layout in ring_logger.h: timestamp_ns (Q), sequence (Q), length (I), level (B), data (256 bytes)
LOG_FMT = "<QQIB256s"
LOG_SIZE = struct.calcsize(LOG_FMT)

async def ship(path: Path, endpoint: str, poll_interval: float = 0.1):
    async with aiohttp.ClientSession() as session:
        # naive tail implementation
        with path.open("rb") as f:
            f.seek(0, 2)
            while True:
                chunk = f.read(LOG_SIZE)
                if not chunk or len(chunk) < LOG_SIZE:
                    await asyncio.sleep(poll_interval)
                    continue
                ts, seq, length, level, data = struct.unpack(LOG_FMT, chunk)
                payload = {
                    "timestamp_ns": ts,
                    "sequence": seq,
                    "length": length,
                    "level": int(level),
                    "raw": data[:length].hex(),
                }
                try:
                    async with session.post(endpoint, json=payload) as resp:
                        await resp.text()
                except Exception:
                    await asyncio.sleep(0.5)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--binlog", default="/tmp/fairtrade.binlog")
    p.add_argument("--endpoint", default="http://127.0.0.1:8000/logs")
    args = p.parse_args()
    asyncio.run(ship(Path(args.binlog), args.endpoint))

if __name__ == "__main__":
    main()
