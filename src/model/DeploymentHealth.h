#pragma once

#include "Deployment.h"

#include <nlohmann/json.hpp>
#include <string>

namespace dr::model {

struct DeploymentHealth {
    int         id                = 0;
    int         deployment_id     = 0;
    std::string service_name;
    std::string status            = "Unknown";
    std::string checked_at;
    int         response_time_ms  = 0;
    std::string message;

    static DeploymentHealth fromJson(const nlohmann::json& j) {
        DeploymentHealth dh;
        auto attr = j.contains("attributes") ? j["attributes"] : j;
        dh.id               = jint(j, "id");
        dh.deployment_id    = jint(attr, "deployment_id");
        dh.service_name     = jstr(attr, "service_name");
        dh.status           = jstr(attr, "status", "Unknown");
        dh.checked_at       = jstr(attr, "checked_at");
        dh.response_time_ms = jint(attr, "response_time_ms");
        dh.message          = jstr(attr, "message");
        return dh;
    }

    nlohmann::json toJson() const {
        return {
            {"deployment_id",    deployment_id},
            {"service_name",     service_name},
            {"status",           status},
            {"response_time_ms", response_time_ms},
            {"message",          message}
        };
    }
};

} // namespace dr::model
