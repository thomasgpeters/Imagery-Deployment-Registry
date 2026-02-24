#include "model/Deployment.h"
#include "model/DeploymentImage.h"
#include "model/DeploymentPort.h"
#include "model/DeploymentEnvVar.h"
#include "model/DeploymentTarget.h"
#include "model/DeploymentHealth.h"
#include "model/DeploymentLog.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace dr::model;
using njson = nlohmann::json;

// ---------------------------------------------------------------------------
// Deployment
// ---------------------------------------------------------------------------

void test_deployment_round_trip()
{
    njson j = {
        {"id", "42"},
        {"attributes", {
            {"environment_name", "dev-local"},
            {"stack_name", "als-three-tier"},
            {"pipeline_name", "default"},
            {"target", "DockerCompose"},
            {"provider", "Local"},
            {"status", "Running"},
            {"deployed_by", "admin"},
            {"deployed_at", "2026-02-12T11:55:00Z"},
            {"finished_at", "2026-02-12T11:55:45Z"},
            {"compose_content", "version: '3'"},
            {"compose_project_name", "vcp-dev"},
            {"version_label", "v0.9.1"},
            {"notes", "test"}
        }}
    };

    auto d = Deployment::fromJson(j);
    assert(d.id == 42);
    assert(d.environment_name == "dev-local");
    assert(d.stack_name == "als-three-tier");
    assert(d.status == "Running");
    assert(d.version_label == "v0.9.1");

    auto out = d.toJson();
    assert(out["environment_name"] == "dev-local");
    assert(out["status"] == "Running");

    std::cout << "  PASS: test_deployment_round_trip" << std::endl;
}

// ---------------------------------------------------------------------------
// DeploymentImage
// ---------------------------------------------------------------------------

void test_deployment_image_qualified_name()
{
    njson j = {
        {"id", 1},
        {"attributes", {
            {"deployment_id", 1},
            {"tier", "Frontend"},
            {"service_name", "wt-frontend"},
            {"image_name", "wt-app-base"},
            {"tag", "ubuntu22"},
            {"digest", ""},
            {"registry_host", ""},
            {"registry_kind", ""}
        }}
    };

    auto img = DeploymentImage::fromJson(j);
    assert(img.qualifiedName() == "wt-app-base:ubuntu22");

    // With registry host
    img.registry_host = "docker.io";
    assert(img.qualifiedName() == "docker.io/wt-app-base:ubuntu22");

    std::cout << "  PASS: test_deployment_image_qualified_name" << std::endl;
}

// ---------------------------------------------------------------------------
// DeploymentPort
// ---------------------------------------------------------------------------

void test_deployment_port_mapping_string()
{
    njson j = {
        {"id", 1},
        {"attributes", {
            {"deployment_id", 1},
            {"service_name", "wt-frontend"},
            {"container_port", 80},
            {"host_port", 8080},
            {"protocol", "tcp"}
        }}
    };

    auto p = DeploymentPort::fromJson(j);
    assert(p.mappingString() == "8080:80/tcp");

    std::cout << "  PASS: test_deployment_port_mapping_string" << std::endl;
}

// ---------------------------------------------------------------------------
// DeploymentEnvVar
// ---------------------------------------------------------------------------

void test_deployment_env_var_masking()
{
    DeploymentEnvVar v;
    v.var_name = "POSTGRES_PASSWORD";
    v.var_value = "supersecret";
    v.is_secret = true;

    assert(v.displayValue() == "********");

    v.is_secret = false;
    assert(v.displayValue() == "supersecret");

    std::cout << "  PASS: test_deployment_env_var_masking" << std::endl;
}

// ---------------------------------------------------------------------------
// DeploymentTarget
// ---------------------------------------------------------------------------

void test_deployment_target_round_trip()
{
    njson j = {
        {"id", "5"},
        {"attributes", {
            {"deployment_id", 1},
            {"target_type", "Kubernetes"},
            {"provider", "AKS"},
            {"context_name", "aks-imagery-prod"},
            {"namespace", "vcp-prod"},
            {"compose_file", ""},
            {"project_name", ""},
            {"provider_config", R"({"resourceGroup":"imagery-prod-rg"})"}
        }}
    };

    auto dt = DeploymentTarget::fromJson(j);
    assert(dt.id == 5);
    assert(dt.target_type == "Kubernetes");
    assert(dt.provider == "AKS");
    assert(dt.context_name == "aks-imagery-prod");
    assert(dt.namespace_name == "vcp-prod");

    std::cout << "  PASS: test_deployment_target_round_trip" << std::endl;
}

// ---------------------------------------------------------------------------
// DeploymentLog
// ---------------------------------------------------------------------------

void test_deployment_log_round_trip()
{
    njson j = {
        {"id", 1},
        {"attributes", {
            {"deployment_id", 1},
            {"action", "Deploy"},
            {"status", "Succeeded"},
            {"message", "All services started"},
            {"created_at", "2026-02-12T11:55:45Z"},
            {"created_by", "admin"}
        }}
    };

    auto dl = DeploymentLog::fromJson(j);
    assert(dl.action == "Deploy");
    assert(dl.status == "Succeeded");
    assert(dl.created_by == "admin");

    auto out = dl.toJson();
    assert(out["action"] == "Deploy");

    std::cout << "  PASS: test_deployment_log_round_trip" << std::endl;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "Deployment Registry — Model Tests" << std::endl;
    std::cout << "==================================" << std::endl;

    test_deployment_round_trip();
    test_deployment_image_qualified_name();
    test_deployment_port_mapping_string();
    test_deployment_env_var_masking();
    test_deployment_target_round_trip();
    test_deployment_log_round_trip();

    std::cout << std::endl << "All tests passed." << std::endl;
    return 0;
}
