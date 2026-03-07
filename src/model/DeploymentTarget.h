#pragma once

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
        auto attr = j.value("attributes", j);
        if (j.contains("id")) {
            if (j["id"].is_number())
                dt.id = j["id"].get<int>();
            else if (j["id"].is_string())
                try { dt.id = std::stoi(j["id"].get<std::string>()); } catch (...) {}
        }
        dt.deployment_id   = attr.value("deployment_id", 0);
        dt.target_type     = attr.value("target_type", "DockerCompose");
        dt.provider        = attr.value("provider", "Local");
        dt.context_name    = attr.value("context_name", "");
        dt.namespace_name  = attr.value("namespace", "default");
        dt.compose_file    = attr.value("compose_file", "");
        dt.project_name    = attr.value("project_name", "");
        dt.provider_config = attr.value("provider_config", "");
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
