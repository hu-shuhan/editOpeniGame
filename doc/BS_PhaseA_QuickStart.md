# BS Phase-A Quick Start (.vtk/.vtu direct delivery)

This document provides a 1-2 day execution path for phase A:
- server stores wasm/web assets and vtk/vtu files
- browser requests model from server API
- browser loads bytes into wasm memory and renders

## 1. What goes to server

Deploy these files/directories to the Linux server:

- Static web files (served by Nginx):
  - `index.html`
  - `iGameWasmDemo.js`
  - `iGameWasmDemo.wasm`
- Model files directory:
  - `.vtk` and `.vtu` files only
- API service files:
  - `Script/bs_phase_a_api.py`
  - `Script/requirements_phase_a.txt`
- Nginx site config:
  - `Script/deploy/nginx/igame-bs-phase-a.conf`

Recommended server layout:

- `/opt/igame-bs/static/` -> web static files
- `/opt/igame-bs/models/` -> model files
- `/opt/igame-bs/backend/` -> python service files

## 2. What goes to browser machine

Required:
- A modern browser (Chrome/Edge/Firefox)
- No wasm/js/model files are required locally in production use

Optional for developer debugging only:
- local copy of web files

## 3. Server setup commands (Alibaba Cloud Linux)

```bash
sudo mkdir -p /opt/igame-bs/static /opt/igame-bs/models /opt/igame-bs/backend

# copy files from your build machine
# static files -> /opt/igame-bs/static/
# model files  -> /opt/igame-bs/models/
# api files    -> /opt/igame-bs/backend/
```

Install Python runtime and dependencies:

```bash
cd /opt/igame-bs/backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements_phase_a.txt
```

Start API service (first validation):

```bash
cd /opt/igame-bs/backend
source .venv/bin/activate
python bs_phase_a_api.py --model-root /opt/igame-bs/models --host 127.0.0.1 --port 18080
```

Install and apply Nginx config:

```bash
sudo cp /opt/igame-bs/backend/igame-bs-phase-a.conf /etc/nginx/conf.d/igame-bs.conf
sudo nginx -t
sudo systemctl restart nginx
```

Open Alibaba security group inbound rule for TCP/80.

## 4. Validation checklist

1. Open `http://<public-ip>/` and ensure page loads.
2. Visit `http://<public-ip>/api/health` and check `{ "ok": true }`.
3. Visit `http://<public-ip>/api/models` and ensure model list is returned.
4. Click remote load in page, model appears in scene.

## 5. Phase-A scope limits

- Supported formats: `.vtk`, `.vtu`
- No server-side conversion for CAS/CGNS/ODB in phase A
- Recommended transport: public IP + HTTP first, then migrate to domain + HTTPS
