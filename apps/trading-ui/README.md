Developer guide — trading-ui

Quick commands:

- Install and run dev server (local dev):
```bash
cd apps/trading-ui
nvm use 20 # optional, ensure Node 20
npm install
npm run dev
```

- Build production bundle (used by containers):
```bash
cd apps/trading-ui
npm install
npm run build
```

- Preview built bundle:
```bash
npx serve apps/trading-ui/dist -p 8080
```

Notes:
- The container build uses Node 20 (`Containerfile.ui`) because Vite v8 requires Node >=20.
- If you change UI assets, rebuild the `fairtrade-ui` image or run the dev server directly.
