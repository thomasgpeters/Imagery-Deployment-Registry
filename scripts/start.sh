#!/usr/bin/env bash
# ============================================================================
# Deployment Registry — Application Launcher
# ============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/env.sh"

# ── CLI overrides ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --port)       HTTP_PORT="$2";   shift 2 ;;
        --address)    HTTP_ADDRESS="$2"; shift 2 ;;
        --docroot)    DOCROOT="$2";      shift 2 ;;
        --als-port)   ALS_PORT="$2"; ALS_URL="http://${ALS_HOST}:${ALS_PORT}/api"; shift 2 ;;
        --log-level)  LOG_LEVEL="$2";    shift 2 ;;
        *)            shift ;;
    esac
done

export ALS_URL

# ── Resolve executable ─────────────────────────────────────────────────────
EXE="${SCRIPT_DIR}/deployment_registry"
if [[ ! -x "${EXE}" ]]; then
    echo "ERROR: executable not found at ${EXE}" >&2
    echo "       Run 'cmake --build build' first." >&2
    exit 1
fi

echo "──────────────────────────────────────────────────"
echo "  Deployment Registry"
echo "  HTTP  : ${HTTP_ADDRESS}:${HTTP_PORT}"
echo "  ALS   : ${ALS_URL}"
echo "  Log   : ${LOG_LEVEL}"
echo "──────────────────────────────────────────────────"

exec "${EXE}" \
    --docroot "${DOCROOT}" \
    --http-address "${HTTP_ADDRESS}" \
    --http-port "${HTTP_PORT}"
