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

### Task Backend Contract (for `community_backend`)

The web frontend now sends task events and renders shared tasks from your backend.

Configure backend URL in the browser before loading the game (optional):

```html
<script>
	window.COMMUNITY_BACKEND_URL = "http://localhost:3000";
</script>
```

If not set, the default is `http://localhost:3000`.

Expected endpoints:

- `GET /tasks`
	- Returns either an array of tasks, or `{ "tasks": [...] }`
- `POST /tasks`
	- Body: `{ "name": string, "room": number, "taskId": number, "completed": false }`
	- Called when a new objective is added in game
- `POST /tasks/complete`
	- Body: `{ "name": string, "room": number, "taskId": number, "completed": true }`
	- Called when an objective is completed in game

Task object fields read by the UI:

- `name` (or `task_name`)
- `room` (or `room_id`)
- `completed` (boolean)
- Optional timestamp shown in UI: `updatedAt`, `updated_at`, `completedAt`, or `completed_at`

The task panel is rendered as a DOM overlay in the top-right of the web build.

### GitHub Pages Deployment

This repository includes an Actions workflow at `.github/workflows/deploy-pages.yml` that builds and deploys `dist_web/` to GitHub Pages on pushes to `main`.

1. Push this repo to GitHub.
2. In GitHub, open Settings -> Pages.
3. Set Source to `GitHub Actions`.
4. Push to `main` (or run the workflow manually from Actions).
5. After deploy completes, your game will be available at:
	 `https://<your-user-or-org>.github.io/<your-repo>/index.html`

Iframe example:

```html
<iframe
	src="https://<your-user-or-org>.github.io/<your-repo>/index.html"
	title="Community"
	width="100%"
	height="720"
	style="border:0;"
	allowfullscreen>
</iframe>
```