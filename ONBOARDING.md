**Fairtrade — Onboarding & Developer Quick Start**

This document lists steps, commands, and notes to get new joiners productive with this repository.

**Prerequisites**
- **Host OS:** macOS (tested). Ensure Xcode Command Line Tools if you plan native builds: `xcode-select --install`.
- **Container runtime:** Podman (rootless) + `podman-compose` (or Docker + Docker Compose). Use Podman instructions in this repo.
- **Node / npm:** Node >=20 for UI builds (we build UI inside container by default). Locally use `nvm` if needed.
- **Python:** 3.11+ for test tooling.

**Repository layout (important paths)**
- `core/trading-engine/` — C++ engine sources, headers, CMakeLists.
  - `include/` — public headers (`ring_logger.h`, `portfolio.h`, `risk_engine.h`, `sbe_messages.h`).
  - `src/` — implementation (`portfolio.cpp`, `risk_engine.cpp`, `ring_logger.cpp`, `engine_main.cpp`).
- `apps/trading-ui/` — React + Vite UI sources; production bundle produced at `apps/trading-ui/dist`.
- `tools/test-suite/` — Python test scripts, `scripts/` contains `market_simulator.py`, `fluent_bit_shipper.py`, and `receiver.py`.
- `Containerfile.engine`, `Containerfile.ui`, `Containerfile.telemetry` — container build files.
- `container-compose.yaml` — compose manifest used during local development.

**Quick start — Build & run full stack (Podman)**
1. Ensure Podman machine is running (rootless VM):
```bash
podman machine start
```
2. Build and start the compose stack (images will be built if missing):
```bash
podman-compose -f container-compose.yaml up --build -d
```
3. Inspect running containers:
```bash
podman ps -a
podman-compose -f container-compose.yaml ps
```
4. Check UI: the UI is exposed on host port `3000` (mapped to container `80`). Quick check:
```bash
curl -I http://localhost:3000
```

**Engine (C++): build & run notes**
- The engine is built inside a container to avoid host toolchain mismatches. The runtime image runs a small demo binary `trading_engine_demo` that writes heartbeat records to `/tmp/fairtrade.binlog`.
- To rebuild the engine image only:
```bash
podman-compose -f container-compose.yaml build engine
podman-compose -f container-compose.yaml up -d engine
```
- Check engine container status and logs:
```bash
podman ps --filter name=fairtrade_engine_1
podman logs --tail 200 fairtrade_engine_1
```
- Binlog on the host: `/tmp/fairtrade.binlog` is mounted into both engine and telemetry containers.

**UI: build & preview**
- We produce production UI bundles inside the build container and serve via Nginx in the runtime image.
- To build locally (if you prefer):
```bash
cd apps/trading-ui
nvm use 20 # optional
npm install
npm run build
```
- To preview the built bundle (if not using compose):
```bash
npx serve apps/trading-ui/dist -p 8080
```

**Python test suite & telemetry**
- A Testplan-based test suite exists under `tools/test-suite/` (requires a venv). To run locally:
```bash
python3 -m venv tools/test-suite/venv
source tools/test-suite/venv/bin/activate
pip install -r tools/test-suite/requirements.txt  # or pip install testplan aiohttp
python -m pytest tools/test-suite  # if pytest harness provided
```
- The telemetry shipper (`fluent_bit_shipper.py`) tails `/tmp/fairtrade.binlog` and POSTs parsed entries to an endpoint.
- During development we added a small `receiver.py` (HTTP server) to capture POSTs. Compose wires telemetry to the `receiver` service.

**Binary log (format)**
- Records are fixed-layout `LogEntry` structs (see `core/trading-engine/include/ring_logger.h`):
  - `timestamp_ns` (Q), `sequence` (Q), `length` (I), `level` (B), `data` (256 bytes)
- The shipper expects this exact layout. Helper scripts in `tools/test-suite/scripts` can create/test records.

**How we debugged common issues**
- Disk full during image pulls/build: check host free space and Podman VM space. Helpful commands:
```bash
df -h /
podman system df
podman machine restart
podman system prune -a
```
- Node engine version mismatch for Vite: update UI `Containerfile.ui` to use `node:20-bullseye` (Vite v8 requires Node >=20).
- C++ compile errors from copying non-copyable atomics: prefer in-place construction (`try_emplace`) and return snapshot structs to avoid moving atomics.
- If the telemetry shipper fails with missing Python packages, rebuild the telemetry image (`Containerfile.telemetry`) to `pip install` the needed deps (e.g., `aiohttp`).

**Useful commands collected**
- Start compose stack (build + detach):
```bash
podman-compose -f container-compose.yaml up --build -d
```
- Show logs for stack services:
```bash
podman-compose -f container-compose.yaml logs -f
podman logs -f fairtrade_receiver_1
podman logs -f fairtrade_telemetry-shipper_1
```
- Inspect a container:
```bash
podman inspect fairtrade_engine_1 --format '{{json .State}}'
podman port fairtrade_ui_1
```

**Next steps for new joiners**
- Read `spec.md` (root) to understand product requirements.
- Run the compose stack locally and confirm UI at `http://localhost:3000`.
- Explore `core/trading-engine/include` to understand message and logging layout.
- If implementing engine features: add unit tests, ensure CMake builds in container, and follow the `try_emplace` pattern for atomic-containing types.

If you want, I can also:
- Add a short README at `apps/trading-ui/` describing how to run the dev server.
- Add a health/readiness endpoint to the engine demo for container supervision.

---
Last updated: 2026-06-06
