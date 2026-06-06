from testplan import Testplan
from testplan.testing.multitest import MultiTest, testsuite, testcase
import subprocess
import tempfile
import time
import os
import threading
import asyncio
import struct


@testsuite
class ToolingSuite:
    @testcase
    def import_scripts(self, env, result):
        # smoke import checks by loading modules from repo-relative paths
        try:
            repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
            ms_path = os.path.join(repo_root, 'tools', 'test-suite', 'scripts', 'market_simulator.py')
            fb_path = os.path.join(repo_root, 'tools', 'test-suite', 'scripts', 'fluent_bit_shipper.py')
            from importlib.machinery import SourceFileLoader
            SourceFileLoader('ms_mod', ms_path).load_module()
            SourceFileLoader('fb_mod', fb_path).load_module()
            result.equal(True, True, "Imported scripts by path")
        except Exception as e:
            import traceback; traceback.print_exc()
            result.fail("Import failed: %s" % (e,))

    @testcase
    def market_simulator_writes_binlog(self, env, result):
        # run the market_simulator as subprocess for a short period and assert file non-empty
        try:
            repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
            ms_script = os.path.join(repo_root, 'tools', 'test-suite', 'scripts', 'market_simulator.py')
            out = tempfile.NamedTemporaryFile(delete=False)
            out.close()
            proc = subprocess.Popen(["/usr/bin/env", "python3", ms_script, "--out", out.name, "--rate", "100" ])
            time.sleep(0.6)
            proc.terminate()
            proc.wait(timeout=1)
            size = os.path.getsize(out.name)
            os.unlink(out.name)
            if size > 0:
                result.equal(True, True, "market_simulator wrote %d bytes" % size)
            else:
                result.fail("market_simulator produced empty file")
        except Exception as e:
            import traceback; traceback.print_exc()
            result.fail(str(e))

    @testcase
    def fluent_bit_shipper_posts(self, env, result):
        # create a dummy log file containing a single fixed-size record and start a tiny HTTP server
        LOG_FMT = "<QQIB256s"
        LOG_SIZE = struct.calcsize(LOG_FMT)
        tmp = tempfile.NamedTemporaryFile(delete=False)
        try:
            # craft a single entry but do not write it yet; we'll append after shipper starts
            ts = int(time.time() * 1e9)
            seq = 1
            length = 10
            level = 2
            data = b'hello12345' + b"\x00" * (256 - 10)
            packed = struct.pack(LOG_FMT, ts, seq, length, level, data)

            received = {}

            # use a simple blocking http.server to receive POSTs
            from http.server import BaseHTTPRequestHandler, HTTPServer

            class Handler(BaseHTTPRequestHandler):
                def do_POST(self):
                    length = int(self.headers.get('Content-Length', 0))
                    body = self.rfile.read(length)
                    try:
                        received['body'] = body.decode()
                    except Exception:
                        received['body'] = body.hex()
                    self.send_response(200)
                    self.end_headers()

            server = HTTPServer(('127.0.0.1', 18000), Handler)
            srv_thread = threading.Thread(target=server.serve_forever, daemon=True)
            srv_thread.start()
            time.sleep(0.2)

            try:
                repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
                shipper_script = os.path.join(repo_root, 'tools', 'test-suite', 'scripts', 'fluent_bit_shipper.py')
                proc = subprocess.Popen(["/usr/bin/env", "python3", shipper_script, "--binlog", tmp.name, "--endpoint", "http://127.0.0.1:18000/logs" ])
                time.sleep(0.2)
                # append the packed entry so the shipper (which tails) will pick it up
                with open(tmp.name, 'ab') as f:
                    f.write(packed)
                    f.flush()
                time.sleep(0.8)
                proc.terminate()
                proc.wait(timeout=1)

                if 'body' in received:
                    result.equal(True, True, "shipper posted: %s" % received['body'])
                else:
                    result.fail("shipper did not post to server")
            finally:
                try:
                    server.shutdown()
                except Exception:
                    pass
        finally:
            try:
                os.unlink(tmp.name)
            except Exception:
                pass


def main():
    plan = Testplan(name="Fairtrade Tools Testplan")
    plan.add(MultiTest(name="tooling_multitest", suites=[ToolingSuite()]))
    plan.run()


if __name__ == '__main__':
    main()
