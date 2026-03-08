-- ============================================================================
-- Deployment Registry — Seed Data
-- ============================================================================
-- Minimal, realistic seed data that reflects the actual VCP three-tier
-- e-commerce stack deployed to the dev-local environment.
--
-- Sources:
--   - VCP seed_data.sql  (environment "dev-local", stack "als-three-tier")
--   - VCP schema.sql     (compose project "vcp-dev", ports 8080/5656/5432)
--
-- Idempotent: uses ON CONFLICT / NOT EXISTS guards so this script is safe
-- to re-run against an existing database.
-- ============================================================================

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT — als-three-tier → dev-local (DockerCompose / Local)
-- ────────────────────────────────────────────────────────────────────────────

INSERT INTO deployment (
    name, environment_name, stack_name, pipeline_name,
    target, provider, status,
    deployed_by, deployed_at, finished_at,
    compose_project_name, version_label, notes
)
SELECT
    'student-onboarding', 'dev-local', 'als-three-tier', 'default',
    'DockerCompose', 'Local', 'Running',
    'admin', '2026-02-12 11:55:00+00'::timestamptz, '2026-02-12 11:55:45+00'::timestamptz,
    'vcp-dev', 'v0.9.1', 'Student onboarding portal — three-tier stack for local development'
WHERE NOT EXISTS (
    SELECT 1 FROM deployment
    WHERE name = 'student-onboarding'
    LIMIT 1
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT — faculty-portal → staging (DockerCompose / ApiInABox)
-- ────────────────────────────────────────────────────────────────────────────

INSERT INTO deployment (
    name, environment_name, stack_name, pipeline_name,
    target, provider, status,
    deployed_by, deployed_at, finished_at,
    compose_project_name, version_label, notes
)
SELECT
    'faculty-portal', 'staging', 'als-three-tier', 'ci-main',
    'DockerCompose', 'ApiInABox', 'Running',
    'jdoe', '2026-02-20 09:30:00+00'::timestamptz, '2026-02-20 09:31:12+00'::timestamptz,
    'faculty-stg', 'v1.2.0', 'Faculty portal staging environment — mirrors production config'
WHERE NOT EXISTS (
    SELECT 1 FROM deployment
    WHERE name = 'faculty-portal'
    LIMIT 1
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT — course-catalog → production (Kubernetes / AKS)
-- ────────────────────────────────────────────────────────────────────────────

INSERT INTO deployment (
    name, environment_name, stack_name, pipeline_name,
    target, provider, status,
    deployed_by, deployed_at, finished_at,
    compose_project_name, version_label, notes
)
SELECT
    'course-catalog', 'production', 'als-three-tier', 'release',
    'Kubernetes', 'AKS', 'Running',
    'deploy-bot', '2026-03-01 14:00:00+00'::timestamptz, '2026-03-01 14:03:22+00'::timestamptz,
    '', 'v2.0.3', 'Course catalog service — production AKS cluster'
WHERE NOT EXISTS (
    SELECT 1 FROM deployment
    WHERE name = 'course-catalog'
    LIMIT 1
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT IMAGES — Three tiers matching VCP seed InfraResource + StackReq
-- ────────────────────────────────────────────────────────────────────────────

INSERT INTO deployment_image (deployment_id, tier, service_name, image_name, tag, registry_host, registry_kind)
SELECT v.*
FROM (VALUES
    (1, 'Frontend',   'wt-frontend',  'wt-app-base',                        'ubuntu22', '',          ''),
    (1, 'Middleware',  'als-api',      'apilogicserver/api_logic_server',    'latest',   'docker.io', 'DockerHub'),
    (1, 'Database',   'postgresql',    'postgres',                           '16',       'docker.io', 'DockerHub')
) AS v(deployment_id, tier, service_name, image_name, tag, registry_host, registry_kind)
WHERE NOT EXISTS (SELECT 1 FROM deployment_image WHERE deployment_id = 1 LIMIT 1);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT PORTS — Published ports for dev-local compose stack
-- ────────────────────────────────────────────────────────────────────────────

INSERT INTO deployment_port (deployment_id, service_name, container_port, host_port, protocol)
SELECT v.*
FROM (VALUES
    (1, 'wt-frontend', 80,   8080, 'tcp'),
    (1, 'als-api',     5656, 5656, 'tcp'),
    (1, 'postgresql',  5432, 5432, 'tcp')
) AS v(deployment_id, service_name, container_port, host_port, protocol)
WHERE NOT EXISTS (SELECT 1 FROM deployment_port WHERE deployment_id = 1 LIMIT 1);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT ENV VARS — Standard compose env vars for the three tiers
-- ────────────────────────────────────────────────────────────────────────────

INSERT INTO deployment_env_var (deployment_id, service_name, var_name, var_value, is_secret)
SELECT v.*
FROM (VALUES
    (1, 'wt-frontend', 'HTTP_PORT',      '8080',      FALSE),
    (1, 'wt-frontend', 'HTTP_ADDRESS',   '0.0.0.0',   FALSE),
    (1, 'als-api',     'APILOGICPROJECT_API_PREFIX',  '/api', FALSE),
    (1, 'als-api',     'APILOGICPROJECT_PORT',        '5656', FALSE),
    (1, 'als-api',     'APILOGICPROJECT_DB_URL',      'postgresql://postgres:p@postgresql:5432/ecommerce_dev', FALSE),
    (1, 'postgresql',  'POSTGRES_DB',       'ecommerce_dev', FALSE),
    (1, 'postgresql',  'POSTGRES_USER',     'postgres',      FALSE),
    (1, 'postgresql',  'POSTGRES_PASSWORD', '********',      TRUE)
) AS v(deployment_id, service_name, var_name, var_value, is_secret)
WHERE NOT EXISTS (SELECT 1 FROM deployment_env_var WHERE deployment_id = 1 LIMIT 1);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT TARGET — dev-local DockerCompose target snapshot
-- ────────────────────────────────────────────────────────────────────────────

INSERT INTO deployment_target (
    deployment_id, target_type, provider,
    context_name, namespace,
    compose_file, project_name, provider_config
)
SELECT
    1, 'DockerCompose', 'Local',
    '', 'default',
    'deploy/docker-compose.yaml', 'vcp-dev', ''
WHERE NOT EXISTS (SELECT 1 FROM deployment_target WHERE deployment_id = 1 LIMIT 1);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT LOG — Initial deploy action
-- ────────────────────────────────────────────────────────────────────────────

INSERT INTO deployment_log (deployment_id, action, status, message, created_at, created_by)
SELECT v.*
FROM (VALUES
    (1, 'Deploy', 'Started',   'Deploying als-three-tier to dev-local via DockerCompose',
     '2026-02-12 11:55:00+00'::timestamptz, 'admin'),
    (1, 'Deploy', 'Succeeded', 'All 3 services started successfully (wt-frontend, als-api, postgresql)',
     '2026-02-12 11:55:45+00'::timestamptz, 'admin')
) AS v(deployment_id, action, status, message, created_at, created_by)
WHERE NOT EXISTS (SELECT 1 FROM deployment_log WHERE deployment_id = 1 LIMIT 1);
