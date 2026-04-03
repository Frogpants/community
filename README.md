### Commands

```bash
# Initialize Project
make clean

# Build Native Project
make

# Run Native Project
make run
```

### WebAssembly Build (iframe-ready)

```bash
# Build wasm bundle into dist_web/
make web

# Serve dist_web/ at http://localhost:8080
make web-run
```

The web output is generated at `dist_web/index.html` and can be embedded directly:

```html
<iframe
	src="https://your-host/community/index.html"
	title="Community"
	width="100%"
	height="720"
	style="border:0;"
	allowfullscreen>
</iframe>
```

Notes:
- `make web` now auto-detects and sources a local `.emsdk` checkout if present.
- If `em++` is still missing, it will try a one-time local bootstrap via `.emsdk/emsdk install latest` and `.emsdk/emsdk activate latest`.
- Multiplayer is disabled in the web build right now (single-player only).