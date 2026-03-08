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
from sqlalchemy import and_                        # type: ignore


def declare_logic():
    """Called once at ALS startup to register rules with LogicBank."""

    # ── Normalize timezone-aware datetimes before ALS's strptime parser ───
    Rule.early_row_event(on_class=models.Deployment,
                         calling=_normalize_datetimes)

    # ── Deduplicate on insert — find existing by (name, env, stack) ───────
    Rule.early_row_event(on_class=models.Deployment,
                         calling=_deduplicate_deployment)

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
# Deduplication
# ---------------------------------------------------------------------------

def _deduplicate_deployment(row, old_row, logic_row):
    """Prevent duplicate deployments for the same (name, environment, stack).

    If an INSERT arrives and a deployment with the same (name,
    environment_name, stack_name) already exists, we copy the incoming
    attributes onto the existing row and cancel the insert by raising
    a redirect.  The caller (VCP) should use PATCH when possible, but
    this guard ensures the registry never holds two records for the
    same logical deployment.

    Note: This is a best-effort application-level guard.  The database
    also enforces a UNIQUE index ``uq_deploy_name_env_stack``.
    """
    if logic_row.ins_upd_dlt != "ins":
        return  # only guard inserts

    if not row.name or not row.environment_name or not row.stack_name:
        return  # incomplete key — let normal validation handle it

    existing = logic_row.session.query(models.Deployment).filter(
        and_(
            models.Deployment.name == row.name,
            models.Deployment.environment_name == row.environment_name,
            models.Deployment.stack_name == row.stack_name,
        )
    ).first()

    if existing is not None:
        # Update the existing record instead of creating a duplicate.
        # Copy mutable fields from the incoming row.
        for col in ("pipeline_name", "target", "provider", "status",
                     "deployed_by", "deployed_at", "finished_at",
                     "compose_content", "compose_project_name",
                     "version_label", "notes"):
            val = getattr(row, col, None)
            if val is not None:
                setattr(existing, col, val)

        # Log the upsert
        _write_log(
            logic_row=logic_row,
            deployment_id=existing.id,
            action="Deploy",
            status="Started",
            message=f"Deployment '{row.name}' upserted (duplicate prevented) — "
                    f"{row.stack_name} → {row.environment_name}",
            created_by=getattr(row, "deployed_by", None) or "system",
        )

        # Cancel the insert by raising — ALS will commit the session
        # changes (the UPDATE to existing) but skip the INSERT.
        from logic_bank.exec_row_logic.logic_row import LogicException  # type: ignore
        raise LogicException(
            f"Duplicate prevented: deployment '{row.name}' in "
            f"{row.environment_name}/{row.stack_name} already exists "
            f"(id={existing.id}). Existing record was updated instead."
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
