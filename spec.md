Technical Specification: Low-Latency Algorithmic Trading Workstation1. Core Engineering Directive (Prompt for Local Agent)"Act as an elite Principal Systems Engineer specializing in low-latency quantitative finance platforms. Generate a complete, un-truncated polyglot monorepo matching the strict directory layout and architectural boundaries detailed below. Implement pure C++20 for the core trading engine, Python 3.11+ using 'uv' for data generation and log shipping, and React/Vite/TypeScript for the management terminal. Every file must be fully written out to the last byte with complete production logic, explicit error checking, zero placeholder comments, and no text truncation."2. Definitive Directory Layouttexttrading-monorepo/
├── core/
│   └── trading-engine/             # C++20 Low-Latency Subsystem
│       ├── include/
│       │   ├── sbe_messages.h      # Fixed-width zero-copy message layouts
│       │   ├── risk_engine.h       # Pre-trade inline limit verification definitions
│       │   ├── portfolio.h         # Multi-account position matrices
│       │   └── ring_logger.h       # Non-blocking lock-free circular logging boundaries
│       ├── src/
│       │   ├── risk_engine.cpp     # Nanosecond validation algorithms
│       │   ├── portfolio.cpp       # Rolling average cost-basis engine
│       │   └── ring_logger.cpp     # Asynchronous file IO worker thread
│       └── CMakeLists.txt          # Native compiler toolchain profile
├── tools/
│   └── test-suite/                 # Python 3.11+ Tooling & Telemetry
│       ├── scripts/
│       │   ├── market_simulator.py # Live random-walk pricing injector
│       │   └── fluent_bit_shipper.py # Off-path binary-to-JSON log shipper sidecar
│       └── pyproject.toml          # Declarative workspace file via 'uv'
├── apps/
│   └── trading-ui/                 # React 18 / TypeScript Presentation Layer
│       ├── src/
│       │   ├── types/
│       │   │   └── trading.ts      # Mirrored type declarations
│       │   ├── App.tsx             # Analytical state tracking console
│       │   └── main.tsx            # DOM mounting layout
│       ├── index.html              # Primary viewport index
│       ├── package.json            # Node module inventory
│       └── vite.config.ts          # Optimized dev/build port settings
├── infra/
│   └── fluent-bit.conf             # Fluent Bit ingest & output processor topology
├── Containerfile.engine            # Multi-stage C++ native + Python runtime container
├── Containerfile.ui                # Vite production compilation + Nginx static asset runner
└── container-compose.yaml          # Rootless Podman compose architecture manifest
Use code with caution.3. Detailed Component Code SpecificationsA. C++20 Core Execution Subsystem (core/trading-engine/)Simple Binary Encoding (sbe_messages.h): Apply explicit #pragma pack(push, 1) binary packing. Enforce zero runtime heap allocations. Map network incoming arrays using raw byte casting with zero copying.Pre-Trade Risk Engine (risk_engine.h/.cpp): Process inline checks before execution frames change. Intercept orders to validate Max Single Order Value limits and Daily Loss Limits. If limits breach, return a distinct return failure code (-99) immediately.Multi-Tenant Position Matrix (portfolio.h/.cpp): Handle distinct position tracking isolated by a uint64_t account_id. Calculate rolling average volume-weighted cost basis on trades. Support an administrative Daily Sign-off & Lockout API (SECURE_SIGNOFF_2026) that freezes further fills across accounts.Async Non-Blocking BinLog (ring_logger.h/.cpp): Utilize a lock-free circular memory buffer with std::atomic sequence trackers aligned to 64-byte CPU cache lines (alignas(64)) to eliminate false sharing. The core thread deposits execution states into the memory sequence in under 4 nanoseconds, leaving a dedicated background worker to flush uncompressed frames onto disk as an append-only .binlog bundle.B. Python 3.11+ Telemetry & Data Gen Layer (tools/test-suite/)Package Automation (pyproject.toml): Use strict Astral uv declarative formats. Completely avoid standard legacy requirements.txt or setup.py manifests.Asynchronous Log Shipper (fluent_bit_shipper.py): Act as a high-speed telemetry sidecar. Continuously read streaming records from the active .binlog file utilizing the native Python struct module. Unpack the bytes instantly, wrap the schema parameters into flat structural JSON dictionaries, and stream them via asynchronous HTTP POST queries to the logging engine endpoint.Market Feed Data Simulator (market_simulator.py): Implement an automated thread network loop modeling random-walk volatility updates across ticker coordinates (AAPL, NVDA, MSFT), invoking immediate shared library states via ctypes mappings to fuel live ring updates.C. TypeScript UI Portal (apps/trading-ui/)Modern Stack Construction: Build using modern React 18 paired with Vite to eliminate heavy Webpack overhead.Type Safety & Telemetry Streaming: Mirror C++ SBE data fields into explicit type interfaces (trading.ts). Build high-speed tracking tables that stream data straight into atomic state pools without running into UI redraw stalls.Operational Controls: Build administrative panels that connect with the background layer to pass variables, submit orders, and dispatch the closing daily sign-off commands.D. Rootless Podman Deployment Layout (container-compose.yaml)Rootless Operation Isolation: Configure services to run completely isolated in rootless spaces inside lightweight Fedora image stacks.Low-Latency Tuning Presets: Configure shared memory parameters using ipc: host settings. Attach advanced user privileges (cap_add: [NET_ADMIN]) as hooks for kernel bypass engines (Solarflare OpenOnload/DPDK) and deploy using high-throughput network configurations (network_mode: "host") to bypass virtual container network bridges.4. Execution Step BlueprintInstruct your generator to output the file layers, then bring the complete workstation online locally via your terminal:bash# 1. Compile the complete native codebase with your local agent's generation tool
# 2. Boot up the entire container stack using podman-compose
podman-compose up --build -d

# 3. Stream real-time, off-path JSON telemetry lines outputting directly to Fluent Bit
podman logs -f telemetry-shipper-sidecar