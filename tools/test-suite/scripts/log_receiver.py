#!/usr/bin/env python3
"""Simple HTTP receiver to accept JSON POSTs at /logs for local testing."""
import argparse
from aiohttp import web
import asyncio

async def handle_logs(request):
    try:
        data = await request.json()
    except Exception:
        data = await request.text()
    print("RECEIVED:", data)
    return web.Response(status=200, text="ok")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="127.0.0.1")
    p.add_argument("--port", type=int, default=8000)
    args = p.parse_args()

    app = web.Application()
    app.router.add_post('/logs', handle_logs)

    web.run_app(app, host=args.host, port=args.port)

if __name__ == '__main__':
    main()
