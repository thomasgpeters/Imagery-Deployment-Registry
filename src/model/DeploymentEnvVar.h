#pragma once

#include "Deployment.h"

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
        auto attr = j.contains("attributes") ? j["attributes"] : j;
        de.id            = jint(j, "id");
        de.deployment_id = jint(attr, "deployment_id");
        de.service_name  = jstr(attr, "service_name");
        de.var_name      = jstr(attr, "var_name");
        de.var_value     = jstr(attr, "var_value");
        de.is_secret     = jbool(attr, "is_secret");
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
