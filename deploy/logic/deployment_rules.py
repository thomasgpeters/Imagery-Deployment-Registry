"""
ALS Business Rules — Deployment lifecycle sync.

Drop this file into the ApiLogicServer project's ``logic/`` directory so that
rules fire on commits to the ``Deployment`` resource endpoint.

VCP creates / removes deployments by POSTing or DELETEing against the ALS
``/api/Deployment/`` JSON:API endpoint.  These rules react to those commits:

  * **after_insert** — log the new deployment, seed child records
  * **after_update** — log status transitions
  * **after_delete** — (cascade handled by DB FK, but we log it here)

Requires:  ApiLogicServer >= 10.x  (declare_logic / LogicBank)
"""

import datetime as _dt
from logic_bank.logic_bank import Rule           # type: ignore
from database import models                       # type: ignore – ALS generates this


def declare_logic():
    """Called once at ALS startup to register rules with LogicBank."""

    # ── Normalize timezone-aware datetimes before ALS's strptime parser ───
    Rule.early_row_event(on_class=models.Deployment,
                         calling=_normalize_datetimes)

    # ── After INSERT — new deployment arrives from VCP ────────────────────
    Rule.after_insert(
        calling=_on_deployment_created,
        as_condition=lambda row: True,
        on_entity=models.Deployment,
    )

    # ── After UPDATE — status change, version bump, etc. ──────────────────
    Rule.after_update(
        calling=_on_deployment_updated,
        as_condition=lambda row, old_row: row.status != old_row.status,
        on_entity=models.Deployment,
    )

    # ── After DELETE — deployment removed from VCP ────────────────────────
    Rule.after_delete(
        calling=_on_deployment_deleted,
        as_condition=lambda row: True,
        on_entity=models.Deployment,
    )


# ---------------------------------------------------------------------------
# Rule implementations
# ---------------------------------------------------------------------------

def _on_deployment_created(row, old_row, logic_row):
    """Log the creation of a new deployment and set defaults."""
    _write_log(
        logic_row=logic_row,
        deployment_id=row.id,
        action="Deploy",
        status="Started",
        message=f"Deployment '{row.name}' created — "
                f"{row.stack_name} → {row.environment_name} "
                f"via {row.target}/{row.provider}",
        created_by=row.deployed_by or "system",
    )


def _on_deployment_updated(row, old_row, logic_row):
    """Log status transitions (e.g. Deploying → Running, Running → Stopped)."""
    _write_log(
        logic_row=logic_row,
        deployment_id=row.id,
        action="Deploy",
        status="Succeeded" if row.status == "Running" else row.status,
        message=f"Deployment '{row.name}' status changed: "
                f"{old_row.status} → {row.status}",
        created_by=row.deployed_by or "system",
    )


def _on_deployment_deleted(row, old_row, logic_row):
    """
    Log the teardown.  Child records (images, ports, env vars, etc.) are
    cascade-deleted by the database FK constraints — no extra cleanup needed.
    """
    _write_log(
        logic_row=logic_row,
        deployment_id=row.id,
        action="Teardown",
        status="Succeeded",
        message=f"Deployment '{row.name}' removed from registry",
        created_by=row.deployed_by or "system",
    )


# ---------------------------------------------------------------------------
# Datetime normalisation
# ---------------------------------------------------------------------------

def _normalize_datetimes(row, old_row, logic_row):
    """Convert timezone-aware datetime strings to proper Python datetimes.

    ALS's default strptime uses ``%Y-%m-%d %H:%M:%S`` which chokes on the
    ``+00:00`` suffix that VCP sends.  We parse the value with
    ``fromisoformat()`` (handles both ``+00:00`` and ``Z`` suffixes) so
    SQLAlchemy receives a real datetime object instead of an unparseable
    string.
    """
    for col in ("deployed_at", "finished_at"):
        val = getattr(row, col, None)
        if isinstance(val, str) and val:
            try:
                setattr(row, col, _dt.datetime.fromisoformat(val))
            except (ValueError, TypeError):
                pass  # leave as-is; let ALS apply its own fallback


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _write_log(*, logic_row, deployment_id, action, status, message, created_by):
    """Insert a DeploymentLog row inside the current ALS transaction."""
    log_entry = models.DeploymentLog()
    log_entry.deployment_id = deployment_id
    log_entry.action        = action
    log_entry.status        = status
    log_entry.message       = message
    log_entry.created_by    = created_by
    log_entry.created_at    = _dt.datetime.utcnow()
    logic_row.session.add(log_entry)
