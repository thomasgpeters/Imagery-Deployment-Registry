#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace dr::model {

struct Deployment {
    int         id                  = 0;
    std::string environment_name;
    std::string stack_name;
    std::string pipeline_name;
    std::string target              = "DockerCompose";
    std::string provider            = "Local";
    std::string status              = "Pending";
    std::string deployed_by;
    std::string deployed_at;
    std::string finished_at;
    std::string compose_content;
    std::string compose_project_name;
    std::string version_label;
    std::string notes;

    static Deployment fromJson(const nlohmann::json& j) {
        Deployment d;
        auto attr = j.value("attributes", j);
        if (j.contains("id")) {
            if (j["id"].is_number())
                d.id = j["id"].get<int>();
            else if (j["id"].is_string())
                try { d.id = std::stoi(j["id"].get<std::string>()); } catch (...) {}
        }
        d.environment_name     = attr.value("environment_name", "");
        d.stack_name           = attr.value("stack_name", "");
        d.pipeline_name        = attr.value("pipeline_name", "");
        d.target               = attr.value("target", "DockerCompose");
        d.provider             = attr.value("provider", "Local");
        d.status               = attr.value("status", "Pending");
        d.deployed_by          = attr.value("deployed_by", "");
        d.deployed_at          = attr.value("deployed_at", "");
        d.finished_at          = attr.value("finished_at", "");
        d.compose_content      = attr.value("compose_content", "");
        d.compose_project_name = attr.value("compose_project_name", "");
        d.version_label        = attr.value("version_label", "");
        d.notes                = attr.value("notes", "");
        return d;
    }

    nlohmann::json toJson() const {
        return {
            {"environment_name",     environment_name},
            {"stack_name",           stack_name},
            {"pipeline_name",        pipeline_name},
            {"target",               target},
            {"provider",             provider},
            {"status",               status},
            {"deployed_by",          deployed_by},
            {"compose_content",      compose_content},
            {"compose_project_name", compose_project_name},
            {"version_label",        version_label},
            {"notes",                notes}
        };
    }
};

} // namespace dr::model
