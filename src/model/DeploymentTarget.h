#pragma once

#include "Deployment.h"

#include <nlohmann/json.hpp>
#include <string>

namespace dr::model {

struct DeploymentTarget {
    int         id              = 0;
    int         deployment_id   = 0;
    std::string target_type     = "DockerCompose";
    std::string provider        = "Local";
    std::string context_name;
    std::string namespace_name  = "default";
    std::string compose_file;
    std::string project_name;
    std::string provider_config;

    static DeploymentTarget fromJson(const nlohmann::json& j) {
        DeploymentTarget dt;
        auto attr = j.contains("attributes") ? j["attributes"] : j;
        dt.id              = jint(j, "id");
        dt.deployment_id   = jint(attr, "deployment_id");
        dt.target_type     = jstr(attr, "target_type", "DockerCompose");
        dt.provider        = jstr(attr, "provider", "Local");
        dt.context_name    = jstr(attr, "context_name");
        dt.namespace_name  = jstr(attr, "namespace", "default");
        dt.compose_file    = jstr(attr, "compose_file");
        dt.project_name    = jstr(attr, "project_name");
        dt.provider_config = jstr(attr, "provider_config");
        return dt;
    }

    nlohmann::json toJson() const {
        return {
            {"deployment_id",   deployment_id},
            {"target_type",     target_type},
            {"provider",        provider},
            {"context_name",    context_name},
            {"namespace",       namespace_name},
            {"compose_file",    compose_file},
            {"project_name",    project_name},
            {"provider_config", provider_config}
        };
    }
};

} // namespace dr::model
