(function () {
    "use strict";

    const vtkPlane = `# vtk DataFile Version 3.0
iGameVis integration scalar plane
ASCII
DATASET POLYDATA
POINTS 9 float
-1 -1 0  0 -1 0  1 -1 0
-1  0 0  0  0 0  1  0 0
-1  1 0  0  1 0  1  1 0
POLYGONS 8 32
3 0 1 4
3 0 4 3
3 1 2 5
3 1 5 4
3 3 4 7
3 3 7 6
3 4 5 8
3 4 8 7
POINT_DATA 9
SCALARS Temperature float 1
LOOKUP_TABLE default
0 12 24 36 48 60 72 84 100
`;

    let config = {};
    let api = null;
    let animationStarted = false;

    function log(message) {
        const element = document.getElementById("log");
        if (element) {
            const line = `[${new Date().toLocaleTimeString()}] ${message}`;
            element.textContent = element.textContent === "Waiting for WASM..."
                ? line
                : `${line}\n${element.textContent}`;
        }
    }

    function setStatus(message, ready) {
        const status = document.getElementById("status");
        if (!status) {
            return;
        }
        status.textContent = message;
        status.classList.toggle("ready", Boolean(ready));
    }

    function draw() {
        if (api) {
            api.renderFrame();
        }
    }

    function startRenderLoop() {
        if (animationStarted) {
            return;
        }
        animationStarted = true;
        const render = () => {
            if (!animationStarted || !api) {
                return;
            }
            api.renderFrame();
            requestAnimationFrame(render);
        };
        requestAnimationFrame(render);
    }

    function resizeCanvas() {
        if (!api) {
            return;
        }
        const canvas = document.getElementById("canvas");
        const bounds = canvas.getBoundingClientRect();
        const ratio = window.devicePixelRatio || 1;
        const width = Math.max(1, Math.round(bounds.width * ratio));
        const height = Math.max(1, Math.round(bounds.height * ratio));
        canvas.width = width;
        canvas.height = height;
        api.setSize(width, height);
        draw();
    }

    function loadBuiltInModel() {
        const bytes = new TextEncoder().encode(vtkPlane);
        const loaded = api.loadVtkFromMemEx(bytes, "integration-plane.vtk", true);
        if (!loaded) {
            throw new Error(api.getLastErrorJson());
        }
        const scalarApplied = api.setScalarField(0, 0, -1);
        if (!scalarApplied) {
            throw new Error(api.getLastErrorJson());
        }
        api.resetCamera();
        api.viewIsometric();
        api.setSurfaceShadingMode(1);
    }

    function bindCanvasNavigation() {
        const canvas = document.getElementById("canvas");
        const event = { press: 0, move: 1, release: 2, wheel: 3 };
        let active = false;
        let button = 0;
        let selectionPointer = false;
        let selectionPressPoint = null;
        let selectionPointerMoved = false;
        const selectionClickMoveThreshold = 5;
        const point = (e) => {
            const rect = canvas.getBoundingClientRect();
            const ratioX = canvas.width / rect.width;
            const ratioY = canvas.height / rect.height;
            return {
                x: (e.clientX - rect.left) * ratioX,
                y: (e.clientY - rect.top) * ratioY
            };
        };
        const send = (type, activeButton, e, delta, overridePoint) => {
            const p = overridePoint || point(e);
            api.sendMouseEvent(type, activeButton, p.x, p.y, delta || 0);
        };

        canvas.addEventListener("contextmenu", (e) => e.preventDefault());
        canvas.addEventListener("pointerdown", (e) => {
            e.preventDefault();
            active = true;
            button = e.button === 2 ? 2 : e.button === 1 ? 4 : 1;
            selectionPointer = api.getSelectionMode && api.getSelectionMode() !== 0 && button === 1;
            selectionPressPoint = selectionPointer ? point(e) : null;
            selectionPointerMoved = false;
            if (selectionPointer) {
                button = 4;
            }
            canvas.setPointerCapture(e.pointerId);
            send(event.press, button, e, 0);
        });
        canvas.addEventListener("pointermove", (e) => {
            if (active) {
                if (selectionPointer && selectionPressPoint) {
                    const current = point(e);
                    if (Math.hypot(current.x - selectionPressPoint.x, current.y - selectionPressPoint.y)
                        > selectionClickMoveThreshold) {
                        selectionPointerMoved = true;
                    }
                }
                send(event.move, 0, e, 0);
            }
        });
        canvas.addEventListener("pointerup", (e) => {
            if (!active) {
                return;
            }
            send(event.release, button, e, 0,
                selectionPointer && !selectionPointerMoved ? selectionPressPoint : null);
            if (selectionPointer && !selectionPointerMoved && config.onSelectionChanged) {
                config.onSelectionChanged(api);
            }
            active = false;
            selectionPointer = false;
            selectionPressPoint = null;
            selectionPointerMoved = false;
        });
        canvas.addEventListener("wheel", (e) => {
            e.preventDefault();
            send(event.wheel, 0, e, e.deltaY < 0 ? 120 : -120);
        }, { passive: false });
    }

    window.IGameDemo = {
        configure(nextConfig) {
            config = nextConfig || {};
        },
        api() {
            return api;
        },
        draw,
        log,
        call(label, callback) {
            try {
                const value = callback(api);
                log(`${label} -> ${value}`);
                draw();
                return value;
            } catch (error) {
                log(`${label} failed: ${error.message || error}`);
                return 0;
            }
        }
    };

    window.Module = {
        canvas: document.getElementById("canvas"),
        onAbort(reason) {
            setStatus("WASM aborted", false);
            log(`abort: ${reason}`);
        },
        onRuntimeInitialized() {
            try {
                api = Module.iGameWeb;
                api.init();
                resizeCanvas();
                loadBuiltInModel();
                bindCanvasNavigation();
                startRenderLoop();
                setStatus("Ready - built-in scalar model loaded", true);
                log("api.init() / loadVtkFromMemEx() / setScalarField() succeeded");
                if (config.onReady) {
                    config.onReady(api, window.IGameDemo);
                }
                draw();
            } catch (error) {
                setStatus("Initialization failed", false);
                log(error.message || String(error));
            }
        }
    };

    window.addEventListener("resize", resizeCanvas);
}());
