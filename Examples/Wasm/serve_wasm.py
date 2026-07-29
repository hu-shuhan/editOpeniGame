from argparse import ArgumentParser
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
from urllib.parse import urlparse


BUILD_ID = "@IGAME_WASM_BUILD_ID@"
MEMORY_PROFILE = "@IGAME_WASM_MEMORY_PROFILE@"


class WasmRequestHandler(SimpleHTTPRequestHandler):
    model_file: Path | None = None

    def do_GET(self) -> None:
        request_path = urlparse(self.path).path
        if request_path == "/api/build":
            payload = json.dumps(
                {
                    "buildId": BUILD_ID,
                    "memoryProfile": MEMORY_PROFILE,
                }
            ).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return
        if request_path == "/api/models":
            items = []
            if self.model_file is not None:
                items.append({"id": "local", "name": self.model_file.name})
            payload = json.dumps({"items": items}).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)
            return
        if request_path == "/api/models/local/file" and self.model_file is not None:
            try:
                payload_size = self.model_file.stat().st_size
                self.send_response(200)
                self.send_header("Content-Type", "application/octet-stream")
                self.send_header("Content-Length", str(payload_size))
                self.send_header("X-Model-Name", self.model_file.name)
                self.end_headers()
                with self.model_file.open("rb") as input_file:
                    while chunk := input_file.read(1024 * 1024):
                        self.wfile.write(chunk)
            except (BrokenPipeError, ConnectionResetError):
                pass
            return
        super().do_GET()

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        self.send_header("X-iGame-Build-Id", BUILD_ID)
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        super().end_headers()


def main() -> None:
    parser = ArgumentParser()
    parser.add_argument("--directory", type=Path, default=Path.cwd())
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--model-file", type=Path)
    args = parser.parse_args()

    directory = args.directory.resolve()
    WasmRequestHandler.model_file = (
        args.model_file.resolve()
        if args.model_file is not None
        else None
    )
    handler = partial(WasmRequestHandler, directory=str(directory))
    server = ThreadingHTTPServer((args.bind, args.port), handler)
    print(f"Serving {directory} at http://{args.bind}:{args.port}/")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
