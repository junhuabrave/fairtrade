#!/usr/bin/env python3
"""Simple market data random-walk simulator that writes packed MarketDataTick records to a binlog file."""
import argparse
import asyncio
import random
import struct
import os
import time
from pathlib import Path

# Matches struct in sbe_messages.h MarketDataTick packed as:
# uint64_t symbol_id; uint64_t timestamp_ns; int64_t bid_price; int64_t ask_price; uint32_t bid_size; uint32_t ask_size; uint32_t last_price; uint32_t volume
FMT = "<QQqqIIII"  # little-endian: uint64, uint64, int64, int64, uint32, uint32, uint32, uint32
RECORD_SIZE = struct.calcsize(FMT)

TICKERS = {"AAPL": 1, "NVDA": 2, "MSFT": 3}

async def simulate(path: Path, rate_hz: float = 100.0):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("ab") as f:
        state = {sym: 10000 for sym in TICKERS}  # price in fixed int (e.g., price * 100)
        while True:
            for sym, sid in TICKERS.items():
                # random walk
                step = random.randint(-5, 5)
                state[sym] = max(1, state[sym] + step)
                ts = int(time.time() * 1_000_000_000)
                bid = state[sym] - 1
                ask = state[sym] + 1
                bid_size = random.randint(1, 500)
                ask_size = random.randint(1, 500)
                last = state[sym]
                vol = random.randint(1, 1000)
                packed = struct.pack(FMT, sid, ts, bid, ask, bid_size, ask_size, last, vol)
                f.write(packed)
                f.flush()
                try:
                    os.fsync(f.fileno())
                except Exception:
                    pass
            await asyncio.sleep(1.0 / rate_hz)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--out", default="/tmp/fairtrade.binlog")
    p.add_argument("--rate", type=float, default=200.0)
    args = p.parse_args()
    asyncio.run(simulate(Path(args.out), args.rate))

if __name__ == "__main__":
    main()
