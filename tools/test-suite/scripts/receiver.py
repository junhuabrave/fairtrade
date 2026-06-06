#!/usr/bin/env python3
from http.server import BaseHTTPRequestHandler, HTTPServer
import sys

class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        length = int(self.headers.get('content-length', 0))
        body = self.rfile.read(length)
        print('POST', self.path)
        print(body.decode('utf-8', errors='replace'))
        sys.stdout.flush()
        self.send_response(200)
        self.end_headers()

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    server = HTTPServer(('0.0.0.0', port), Handler)
    print('Receiver listening on', port)
    server.serve_forever()
