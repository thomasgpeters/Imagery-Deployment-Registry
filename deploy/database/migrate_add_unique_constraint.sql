-- Migration: Add unique constraint to prevent duplicate deployments
-- Run this against an existing database to add deduplication.
--
-- Before running, check for existing duplicates:
--   SELECT name, environment_name, stack_name, COUNT(*)
--   FROM deployment
--   GROUP BY name, environment_name, stack_name
--   HAVING COUNT(*) > 1;
--
-- If duplicates exist, resolve them first (keep the most recent):
--   DELETE FROM deployment
--   WHERE id NOT IN (
--       SELECT MAX(id)
--       FROM deployment
--       GROUP BY name, environment_name, stack_name
--   );

CREATE UNIQUE INDEX IF NOT EXISTS uq_deploy_name_env_stack
    ON deployment(name, environment_name, stack_name);
