import argparse
import mimetypes
import re
from pathlib import Path
from typing import List

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from pydantic import BaseModel

SUPPORTED_EXTENSIONS = {".vtk", ".vtu", ".igc"}


class ModelItem(BaseModel):
    id: str
    name: str
    format: str
    sizeBytes: int


class ModelListResponse(BaseModel):
    items: List[ModelItem]


def collect_models(model_dir: Path) -> List[ModelItem]:
    items: List[ModelItem] = []
    for path in sorted(model_dir.iterdir()):
        if not path.is_file():
            continue
        suffix = path.suffix.lower()
        if suffix not in SUPPORTED_EXTENSIONS:
            continue
        stat = path.stat()
        items.append(
            ModelItem(
                id=path.name,
                name=path.stem,
                format=suffix.lstrip("."),
                sizeBytes=stat.st_size,
            )
        )
    return items


def build_app(model_root: Path, allow_origins: List[str]) -> FastAPI:
    app = FastAPI(title="iGame BS Phase-A API", version="1.0.0")

    app.add_middleware(
        CORSMiddleware,
        allow_origins=allow_origins,
        allow_credentials=True,
        allow_methods=["GET"],
        allow_headers=["*"],
    )

    @app.get("/api/health")
    def health() -> dict:
        return {"ok": True}

    @app.get("/api/models", response_model=ModelListResponse)
    def list_models() -> ModelListResponse:
        if not model_root.exists() or not model_root.is_dir():
            raise HTTPException(status_code=500, detail=f"Model directory not found: {model_root}")
        return ModelListResponse(items=collect_models(model_root))

    @app.get("/api/models/{model_id}/file")
    def get_model_file(model_id: str) -> FileResponse:
        file_path = (model_root / model_id).resolve()
        try:
            file_path.relative_to(model_root.resolve())
        except ValueError as exc:
            raise HTTPException(status_code=400, detail="Invalid model id") from exc

        if not file_path.exists() or not file_path.is_file():
            raise HTTPException(status_code=404, detail="Model not found")

        suffix = file_path.suffix.lower()
        if suffix not in SUPPORTED_EXTENSIONS:
            raise HTTPException(status_code=400, detail="Only .vtk/.vtu is supported in phase A")

        media_type = mimetypes.guess_type(file_path.name)[0] or "application/octet-stream"
        headers = {
            "X-Model-Name": file_path.name,
            "X-Model-Format": suffix.lstrip("."),
            "Cache-Control": "no-store",
        }
        return FileResponse(path=file_path, media_type=media_type, filename=file_path.name, headers=headers)
    
    @app.get("/api/series/{series_id}/manifest")
    def get_series_manifest(series_id: str) -> dict:
        if series_id != "comp":
            raise HTTPException(status_code=404, detail="Series not found")

        frames = collect_series_frames(model_root, series_id)
        if not frames:
            raise HTTPException(status_code=404, detail="Series contains no frames")

        return {
            "name": series_id,
            "frames": [
                {
                    "index": index,
                    "time": float(index),
                    "name": path.name,
                    "url": f"/api/series/{series_id}/frame/{index}",
                }
                for index, path in enumerate(frames)
            ],
        }


    @app.get("/api/series/{series_id}/frame/{frame_index}")
    def get_series_frame(series_id: str, frame_index: int) -> FileResponse:
        if series_id != "comp":
            raise HTTPException(status_code=404, detail="Series not found")

        frames = collect_series_frames(model_root, series_id)
        if frame_index < 0 or frame_index >= len(frames):
            raise HTTPException(status_code=404, detail="Frame not found")

        frame_path = frames[frame_index]
        return FileResponse(
            path=frame_path,
            media_type="application/octet-stream",
            filename=frame_path.name,
            headers={
                "X-Model-Name": frame_path.name,
                "X-Model-Format": "igc",
                "Cache-Control": "no-store",
            },
        )

    return app


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Phase-A API service for iGame wasm model delivery")
    parser.add_argument("--model-root", default="/opt/igame-bs/models", help="Directory containing .vtk/.vtu files")
    parser.add_argument("--host", default="127.0.0.1", help="Bind host")
    parser.add_argument("--port", type=int, default=18080, help="Bind port")
    parser.add_argument(
        "--allow-origin",
        action="append",
        default=["*"],
        help="CORS allowed origin, repeatable (default: *)",
    )
    return parser.parse_args()

def natural_sort_key(path: Path):
    return [
        int(part) if part.isdigit() else part.lower()
        for part in re.split(r"(\d+)", path.name)
    ]


def collect_series_frames(model_root: Path, prefix: str):
    return sorted(
        model_root.glob(f"{prefix}_*.igc"),
        key=natural_sort_key,
    )


def main() -> None:
    args = parse_args()
    model_root = Path(args.model_root)
    app = build_app(model_root=model_root, allow_origins=args.allow_origin)

    import uvicorn

    uvicorn.run(app, host=args.host, port=args.port, log_level="info")


if __name__ == "__main__":
    main()
