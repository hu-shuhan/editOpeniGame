import json
import os
import re
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse


SCRIPT_DIR = Path(__file__).resolve().parent
APP_ROOT = SCRIPT_DIR.parent
IS_DEPLOY_LAYOUT = (APP_ROOT / "static").exists() and (APP_ROOT / "models").exists()

if IS_DEPLOY_LAYOUT:
    STATIC_DIR = APP_ROOT / "static"
    INDEX_PATH = STATIC_DIR / "index.html"
    MODELS_DIR = APP_ROOT / "models"
else:
    REPO_ROOT = Path(__file__).resolve().parents[2]
    static_candidates = [
        REPO_ROOT / "out" / "build" / "wasm-release-16g" / "Examples" / "Wasm",
        REPO_ROOT / "out" / "build" / "codex-wasm-debug" / "Examples" / "Wasm",
        REPO_ROOT / "out" / "build" / "wasm-debug" / "Examples" / "Wasm",
    ]
    STATIC_DIR = next(
        (
            path
            for path in static_candidates
            if (
                ((path / "iGameWeb_64.js").exists() and (path / "iGameWeb_64.wasm").exists())
                or ((path / "iGameWeb.js").exists() and (path / "iGameWeb.wasm").exists())
            )
        ),
        static_candidates[0],
    )
    INDEX_PATH = SCRIPT_DIR / "index.html"
    MODELS_DIR = REPO_ROOT / "Examples" / "Models"

MODEL_PATH = MODELS_DIR / "Quad_Plane_Tensor.vtk"


def natural_sort_key(path):
    return [int(part) if part.isdigit() else part.lower() for part in re.split(r"(\d+)", path.name)]


def get_series_paths():
    return sorted(MODELS_DIR.glob("comp_*.igc"), key=natural_sort_key)


class DemoHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(STATIC_DIR), **kwargs)

    def send_bytes(self, payload, content_type, *, source_name=None):
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        if source_name:
            self.send_header("X-Model-Name", source_name)
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        request_path = urlparse(self.path).path
        if request_path in ("/", "/index.html"):
            self.send_bytes(INDEX_PATH.read_bytes(), "text/html; charset=utf-8")
            return
        if request_path == "/api/model":
            if not MODEL_PATH.exists():
                self.send_error(404, "Optional VTK model is not installed")
                return
            self.send_bytes(MODEL_PATH.read_bytes(), "application/octet-stream", source_name=MODEL_PATH.name)
            return
        if request_path == "/api/series/comp/manifest":
            series_paths = get_series_paths()
            frames = [
                {
                    "index": index,
                    "time": float(index),
                    "name": path.name,
                    "url": f"/api/series/comp/frame/{index}",
                }
                for index, path in enumerate(series_paths)
            ]
            payload = json.dumps({"name": "comp", "frames": frames}).encode("utf-8")
            self.send_bytes(payload, "application/json; charset=utf-8")
            return
        if request_path.startswith("/api/series/comp/frame/"):
            try:
                index = int(request_path.rsplit("/", 1)[1])
                if index < 0:
                    raise IndexError
                model_path = get_series_paths()[index]
            except (ValueError, IndexError):
                self.send_error(404, "Unknown comp frame")
                return
            self.send_bytes(model_path.read_bytes(), "application/octet-stream", source_name=model_path.name)
            return
        super().do_GET()


if __name__ == "__main__":
    if (STATIC_DIR / "iGameWeb_64.js").exists() and (STATIC_DIR / "iGameWeb_64.wasm").exists():
        js_name = "iGameWeb_64.js"
        wasm_name = "iGameWeb_64.wasm"
    else:
        js_name = "iGameWeb.js"
        wasm_name = "iGameWeb.wasm"
    if not INDEX_PATH.exists() or not (STATIC_DIR / js_name).exists() or not (STATIC_DIR / wasm_name).exists():
        raise SystemExit(f"Missing index.html, {js_name}, or {wasm_name} in static directory: {STATIC_DIR}")
    series_paths = get_series_paths()
    if len(series_paths) < 2:
        raise SystemExit(f"Expected at least two comp_*.igc frames in {MODELS_DIR}")

    default_host = "0.0.0.0" if IS_DEPLOY_LAYOUT else "127.0.0.1"
    host = os.environ.get("IGAME_HOST", default_host)
    port = int(os.environ.get("IGAME_PORT", "8000"))
    server = ThreadingHTTPServer((host, port), DemoHandler)
    print(f"Serving demo at http://{host}:{port}/index.html")
    print(f"Serving static content from {STATIC_DIR}")
    print(f"Serving IGC series frames: {', '.join(path.name for path in series_paths)}")
    server.serve_forever()
