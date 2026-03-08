-- ============================================================================
-- Deployment Registry — PostgreSQL Schema
-- ============================================================================
-- Persistence layer for the VCP Deployment Registry, managed via
-- ApiLogicServer.  Each table maps to a JSON:API resource endpoint.
--
-- Models mapped:
--   Core:    Deployment
--   Detail:  DeploymentImage, DeploymentPort, DeploymentEnvVar
--   Target:  DeploymentTarget
--   Health:  DeploymentHealth
--   Audit:   DeploymentLog
-- ============================================================================

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT — Core deployment record
-- ────────────────────────────────────────────────────────────────────────────
-- Each row represents a single deployment event: one stack deployed to one
-- environment at a specific point in time.  The compose_content column holds
-- the full docker-compose.yaml snapshot so deployments are reproducible.

CREATE TABLE deployment (
    id                   SERIAL PRIMARY KEY,
    name                 VARCHAR(200) NOT NULL DEFAULT '',
    environment_name     VARCHAR(200) NOT NULL,
    stack_name           VARCHAR(200) NOT NULL,
    pipeline_name        VARCHAR(200) NOT NULL DEFAULT '',
    target               VARCHAR(20)  NOT NULL DEFAULT 'DockerCompose'
                             CHECK (target IN ('DockerCompose','Kubernetes')),
    provider             VARCHAR(30)  NOT NULL DEFAULT 'Local'
                             CHECK (provider IN (
                                 'Local','ApiInABox','AzureAppService',
                                 'GoogleCloudRun','AwsEcs',
                                 'Kind','AKS','EKS','GKE','Generic'
                             )),
    status               VARCHAR(20)  NOT NULL DEFAULT 'Pending'
                             CHECK (status IN (
                                 'Pending','Deploying','Running',
                                 'Stopped','Failed','RolledBack'
                             )),
    deployed_by          VARCHAR(100) NOT NULL DEFAULT '',
    deployed_at          TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    finished_at          TIMESTAMP WITH TIME ZONE,
    compose_content      TEXT         NOT NULL DEFAULT '',
    compose_project_name VARCHAR(200) NOT NULL DEFAULT '',
    version_label        VARCHAR(100) NOT NULL DEFAULT '',
    notes                TEXT         NOT NULL DEFAULT ''
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT IMAGE — Container images used in each deployment
-- ────────────────────────────────────────────────────────────────────────────

CREATE TABLE deployment_image (
    id              SERIAL PRIMARY KEY,
    deployment_id   INTEGER      NOT NULL REFERENCES deployment(id) ON DELETE CASCADE,
    tier            VARCHAR(20)  NOT NULL
                        CHECK (tier IN ('Frontend','Middleware','Database','Tools')),
    service_name    VARCHAR(200) NOT NULL,
    image_name      VARCHAR(500) NOT NULL DEFAULT '',
    tag             VARCHAR(200) NOT NULL DEFAULT 'latest',
    digest          VARCHAR(200) NOT NULL DEFAULT '',
    registry_host   VARCHAR(500) NOT NULL DEFAULT '',
    registry_kind   VARCHAR(20)  NOT NULL DEFAULT ''
                        CHECK (registry_kind IN (
                            '','DockerHub','ACR','ECR','GCR','GHCR',
                            'Harbor','LocalRegistry'
                        ))
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT PORT — Published port mappings snapshot
-- ────────────────────────────────────────────────────────────────────────────

CREATE TABLE deployment_port (
    id              SERIAL PRIMARY KEY,
    deployment_id   INTEGER      NOT NULL REFERENCES deployment(id) ON DELETE CASCADE,
    service_name    VARCHAR(200) NOT NULL,
    container_port  INTEGER      NOT NULL DEFAULT 0,
    host_port       INTEGER      NOT NULL DEFAULT 0,
    protocol        VARCHAR(10)  NOT NULL DEFAULT 'tcp'
                        CHECK (protocol IN ('tcp','udp'))
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT ENV VAR — Environment variable snapshot
-- ────────────────────────────────────────────────────────────────────────────
-- Values whose var_name contains PASSWORD, SECRET, TOKEN, or KEY are
-- expected to be masked at insert time by the application layer.

CREATE TABLE deployment_env_var (
    id              SERIAL PRIMARY KEY,
    deployment_id   INTEGER      NOT NULL REFERENCES deployment(id) ON DELETE CASCADE,
    service_name    VARCHAR(200) NOT NULL,
    var_name        VARCHAR(200) NOT NULL,
    var_value       TEXT         NOT NULL DEFAULT '',
    is_secret       BOOLEAN      NOT NULL DEFAULT FALSE
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT TARGET — Environment target snapshot
-- ────────────────────────────────────────────────────────────────────────────
-- One row per deployment capturing the full target configuration at the
-- moment of deployment.  provider_config holds a JSON blob for any
-- provider-specific settings (Azure subscription, GKE project, etc.).

CREATE TABLE deployment_target (
    id              SERIAL PRIMARY KEY,
    deployment_id   INTEGER      NOT NULL REFERENCES deployment(id) ON DELETE CASCADE,
    target_type     VARCHAR(20)  NOT NULL DEFAULT 'DockerCompose'
                        CHECK (target_type IN ('DockerCompose','Kubernetes')),
    provider        VARCHAR(30)  NOT NULL DEFAULT 'Local',
    context_name    VARCHAR(255) NOT NULL DEFAULT '',
    namespace       VARCHAR(255) NOT NULL DEFAULT 'default',
    compose_file    VARCHAR(500) NOT NULL DEFAULT '',
    project_name    VARCHAR(200) NOT NULL DEFAULT '',
    provider_config TEXT         NOT NULL DEFAULT '',
    UNIQUE (deployment_id)
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT HEALTH — Health check snapshots
-- ────────────────────────────────────────────────────────────────────────────

CREATE TABLE deployment_health (
    id                SERIAL PRIMARY KEY,
    deployment_id     INTEGER      NOT NULL REFERENCES deployment(id) ON DELETE CASCADE,
    service_name      VARCHAR(200) NOT NULL,
    status            VARCHAR(20)  NOT NULL DEFAULT 'Unknown'
                          CHECK (status IN ('Healthy','Unhealthy','Starting','Unknown')),
    checked_at        TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    response_time_ms  INTEGER      NOT NULL DEFAULT 0,
    message           TEXT         NOT NULL DEFAULT ''
);

-- ────────────────────────────────────────────────────────────────────────────
-- DEPLOYMENT LOG — Audit trail of deployment lifecycle actions
-- ────────────────────────────────────────────────────────────────────────────

CREATE TABLE deployment_log (
    id              SERIAL PRIMARY KEY,
    deployment_id   INTEGER      NOT NULL REFERENCES deployment(id) ON DELETE CASCADE,
    action          VARCHAR(20)  NOT NULL
                        CHECK (action IN (
                            'Deploy','Stop','Teardown',
                            'Promote','Rollback','Scale','Restart'
                        )),
    status          VARCHAR(20)  NOT NULL DEFAULT 'Started'
                        CHECK (status IN ('Started','Succeeded','Failed')),
    message         TEXT         NOT NULL DEFAULT '',
    created_at      TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    created_by      VARCHAR(100) NOT NULL DEFAULT ''
);

-- ────────────────────────────────────────────────────────────────────────────
-- INDEXES
-- ────────────────────────────────────────────────────────────────────────────

-- Deployment lookups
CREATE INDEX idx_deploy_env        ON deployment(environment_name);
CREATE INDEX idx_deploy_stack      ON deployment(stack_name);
CREATE INDEX idx_deploy_status     ON deployment(status);
CREATE INDEX idx_deploy_at         ON deployment(deployed_at);
CREATE INDEX idx_deploy_env_stack  ON deployment(environment_name, stack_name);

-- Child table FK indexes
CREATE INDEX idx_dimg_deploy       ON deployment_image(deployment_id);
CREATE INDEX idx_dimg_image        ON deployment_image(image_name);
CREATE INDEX idx_dport_deploy      ON deployment_port(deployment_id);
CREATE INDEX idx_denv_deploy       ON deployment_env_var(deployment_id);
CREATE INDEX idx_dtarget_deploy    ON deployment_target(deployment_id);
CREATE INDEX idx_dhealth_deploy    ON deployment_health(deployment_id, checked_at);
CREATE INDEX idx_dhealth_status    ON deployment_health(deployment_id, status);
CREATE INDEX idx_dlog_deploy       ON deployment_log(deployment_id, created_at);
CREATE INDEX idx_dlog_action       ON deployment_log(action);
