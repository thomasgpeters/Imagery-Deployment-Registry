# Development Log — Imagery Deployment Registry

## 2026-03-07 — Fix bus error crash: Http::Client lifecycle and async safety

**Problem:** Clicking "Detail" in the sidebar (or navigating while async HTTP
requests were in-flight) caused a SIGBUS crash. Two root causes:

1. `Wt::Http::Client` was created as a free-floating `shared_ptr`, not parented
   to the widget tree. A circular reference (signal → lambda → shared_ptr →
   client → signal) kept the client alive after session/widget destruction.
   When the timeout callback fired (~5s), it accessed destroyed widgets → bus
   error.

2. Async error callbacks in `DeploymentListView::refresh()` and
   `DeploymentDetailView::loadDeployment()` modified widgets but did not call
   `triggerUpdate()`, leaving the UI in an inconsistent state.

**Fix:**
- Track `Http::Client` instances in `AlsClient::activeClients_` (owned by
  `MainLayout`). After each callback, `retireClient()` removes the shared_ptr,
  breaking the circular reference. When the session is destroyed, `MainLayout`
  → `AlsClient` → all pending clients are destroyed together.
  (`Wt::Http::Client` is not a `WObject` so `addChild`/`parent` are not
  available.)
- Guard all async callbacks with `WApplication::instance()` null check.
- Call `triggerUpdate()` on both success and error paths.

**Files changed:**
- `src/api/AlsClient.cpp`
- `src/ui/DeploymentListView.cpp`
- `src/ui/DeploymentDetailView.cpp`

---

## 2026-03-07 — Fix JSON id parsing in all model classes

**Commit:** `1b3b229`

**Problem:** All seven model `fromJson()` methods used `j.value("id", 0)` which
throws `nlohmann::type_error::302` when the ALS backend returns `id` as a
string (standard JSON:API behavior). The string-fallback code on the next line
never executed because the exception already aborted.

**Fix:** Replaced with explicit type checking — test `is_number()` first, then
`is_string()`, so both forms are handled without exceptions.

**Files changed:**
- `src/model/Deployment.h`
- `src/model/DeploymentImage.h`
- `src/model/DeploymentPort.h`
- `src/model/DeploymentTarget.h`
- `src/model/DeploymentLog.h`
- `src/model/DeploymentEnvVar.h`
- `src/model/DeploymentHealth.h`

---

## 2026-03-07 — Fix sidebar brand icon XHTML compatibility

**Commit:** `3d3e79c`

**Problem:** Sidebar brand used `<i>` tag which is not valid XHTML in Wt's
strict parsing mode.

**Fix:** Replaced `<i>` with `<span>` for the icon element.

---

## 2026-03-07 — Add .gitignore

**Commit:** `f882b16`

Added `.gitignore` for build artifacts (`build/`) and IDE files.

---

## 2026-03-07 — Fix runtime errors and align with app_model.yaml

**Commit:** `3d72b3f`

- Aligned API base URL with `app_model.yaml` (port 5656)
- Added `ALS_URL` environment variable override for runtime configuration
- Fixed resource name mappings to match ALS backend endpoints

---

## 2026-03-07 — Add app_model.yaml

**Commit:** `b126af8`

Added the ALS backend model definition file (`model/app_model.yaml`) describing
all entities: Deployment, DeploymentImage, DeploymentPort, DeploymentEnvVar,
DeploymentTarget, DeploymentHealth, DeploymentLog.

---

## Earlier fixes — Wt4 API compilation errors

**Commits:** `64d798c`, `9257545`, `a10da8f`, `8f4df04`

- Added FetchContent fallback for nlohmann/json when not found via find_package
- Fixed Wt4 API errors in `DeploymentListView` (deprecated/removed APIs)
- Set `TextFormat::XHTML` for raw HTML rendering in UI widgets
- Fixed `WMenuItem` compilation: added missing `<Wt/WMenuItem.h>` include,
  removed invalid `setTextFormat` calls

---

## Initial commit

**Commit:** `91debe1`

Standalone Wt (C++) project for tracking deployment records, images, ports,
environment variables, targets, health checks, and audit logs against an
ApiLogicServer (ALS) JSON:API backend.
