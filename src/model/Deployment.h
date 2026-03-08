#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace dr::model {

/// Null-safe accessor: returns the field as string, or fallback if missing/null.
inline std::string jstr(const nlohmann::json& obj, const char* key,
                        const std::string& fallback = "")
{
    auto it = obj.find(key);
    if (it == obj.end() || it->is_null()) return fallback;
    if (it->is_string()) return it->get<std::string>();
    return it->dump();  // numeric or bool → string representation
}

/// Null-safe accessor for int fields.
inline int jint(const nlohmann::json& obj, const char* key, int fallback = 0)
{
    auto it = obj.find(key);
    if (it == obj.end() || it->is_null()) return fallback;
    if (it->is_number()) return it->get<int>();
    if (it->is_string()) {
        try { return std::stoi(it->get<std::string>()); } catch (...) {}
    }
    return fallback;
}

/// Null-safe accessor for bool fields.
inline bool jbool(const nlohmann::json& obj, const char* key, bool fallback = false)
{
    auto it = obj.find(key);
    if (it == obj.end() || it->is_null()) return fallback;
    if (it->is_boolean()) return it->get<bool>();
    return fallback;
}

struct Deployment {
    int         id                  = 0;
    std::string name;
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
        auto attr = j.contains("attributes") ? j["attributes"] : j;
        d.id                   = jint(j, "id", jint(attr, "id"));
        d.name                 = jstr(attr, "name");
        d.environment_name     = jstr(attr, "environment_name");
        d.stack_name           = jstr(attr, "stack_name");
        d.pipeline_name        = jstr(attr, "pipeline_name");
        d.target               = jstr(attr, "target", "DockerCompose");
        d.provider             = jstr(attr, "provider", "Local");
        d.status               = jstr(attr, "status", "Pending");
        d.deployed_by          = jstr(attr, "deployed_by");
        d.deployed_at          = jstr(attr, "deployed_at");
        d.finished_at          = jstr(attr, "finished_at");
        d.compose_content      = jstr(attr, "compose_content");
        d.compose_project_name = jstr(attr, "compose_project_name");
        d.version_label        = jstr(attr, "version_label");
        d.notes                = jstr(attr, "notes");
        return d;
    }

    /// Human-readable display name with fallback chain.
    std::string displayName() const {
        if (!name.empty()) return name;
        if (!compose_project_name.empty()) return compose_project_name;
        if (!stack_name.empty() && !environment_name.empty())
            return stack_name + " / " + environment_name;
        if (!stack_name.empty()) return stack_name;
        if (!environment_name.empty()) return environment_name;
        if (!pipeline_name.empty()) return pipeline_name;
        return "Deployment #" + std::to_string(id);
    }

    nlohmann::json toJson() const {
        return {
            {"name",                 name},
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
