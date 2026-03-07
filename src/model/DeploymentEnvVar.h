#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace dr::model {

struct DeploymentEnvVar {
    int         id              = 0;
    int         deployment_id   = 0;
    std::string service_name;
    std::string var_name;
    std::string var_value;
    bool        is_secret       = false;

    static DeploymentEnvVar fromJson(const nlohmann::json& j) {
        DeploymentEnvVar de;
        auto attr = j.value("attributes", j);
        if (j.contains("id")) {
            if (j["id"].is_number())
                de.id = j["id"].get<int>();
            else if (j["id"].is_string())
                try { de.id = std::stoi(j["id"].get<std::string>()); } catch (...) {}
        }
        de.deployment_id = attr.value("deployment_id", 0);
        de.service_name  = attr.value("service_name", "");
        de.var_name      = attr.value("var_name", "");
        de.var_value     = attr.value("var_value", "");
        de.is_secret     = attr.value("is_secret", false);
        return de;
    }

    nlohmann::json toJson() const {
        return {
            {"deployment_id", deployment_id},
            {"service_name",  service_name},
            {"var_name",      var_name},
            {"var_value",     var_value},
            {"is_secret",     is_secret}
        };
    }

    /// Display value — masked if secret.
    std::string displayValue() const {
        return is_secret ? "********" : var_value;
    }
};

} // namespace dr::model
