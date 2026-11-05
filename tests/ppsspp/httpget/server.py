#!/usr/bin/env python3
# Test server for httpget: /ok is 64 KB of (i * 7) & 255, /missing is a 404 (GET and POST),
# /cut announces 64 KB but closes after 1000 bytes.
import http.server, sys

BODY = bytes((i * 7) & 255 for i in range(64 * 1024))

class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/ok":
            self.send_response(200)
            self.send_header("Content-Length", str(len(BODY)))
            self.end_headers()
            self.wfile.write(BODY)
        elif self.path == "/cut":
            self.send_response(200)
            self.send_header("Content-Length", str(len(BODY)))
            self.end_headers()
            self.wfile.write(BODY[:1000])
            self.wfile.flush()
            self.close_connection = True
            self.connection.shutdown(2)
        else:
            self.send_error(404)

    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0)))
        self.send_error(404)

    def log_message(self, fmt, *args):
        sys.stderr.write(fmt % args + "\n")

http.server.ThreadingHTTPServer(("127.0.0.1", int(sys.argv[1]) if len(sys.argv) > 1 else 8123), Handler).serve_forever()
