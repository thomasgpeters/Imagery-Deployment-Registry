#!/usr/bin/env bash
# ============================================================================
# Deployment Registry — Environment Configuration
# ============================================================================
# Source this file before running the application.
# All values can be overridden by exporting the variable before sourcing.
# ============================================================================

# ── Wt HTTP Server ─────────────────────────────────────────────────────────
export HTTP_PORT="${HTTP_PORT:-8090}"
export HTTP_ADDRESS="${HTTP_ADDRESS:-0.0.0.0}"
export DOCROOT="${DOCROOT:-.;/resources,/css,/images,/fonts}"

# ── ApiLogicServer (Deployment Registry backend) ───────────────────────────
export ALS_PORT="${ALS_PORT:-5670}"
export ALS_HOST="${ALS_HOST:-127.0.0.1}"
export ALS_URL="${ALS_URL:-http://${ALS_HOST}:${ALS_PORT}/api}"

# ── Database (for ALS tier) ────────────────────────────────────────────────
export DB_HOST="${DB_HOST:-localhost}"
export DB_PORT="${DB_PORT:-5432}"
export DB_NAME="${DB_NAME:-deployment_registry}"
export DB_USER="${DB_USER:-postgres}"
export DATABASE_URL="${DATABASE_URL:-postgresql://${DB_USER}@${DB_HOST}:${DB_PORT}/${DB_NAME}}"

# ── Logging ────────────────────────────────────────────────────────────────
export LOG_LEVEL="${LOG_LEVEL:-info}"
