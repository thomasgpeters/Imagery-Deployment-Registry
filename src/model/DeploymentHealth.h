#pragma once

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
        auto attr = j.value("attributes", j);
        if (j.contains("id")) {
            if (j["id"].is_number())
                dh.id = j["id"].get<int>();
            else if (j["id"].is_string())
                try { dh.id = std::stoi(j["id"].get<std::string>()); } catch (...) {}
        }
        dh.deployment_id    = attr.value("deployment_id", 0);
        dh.service_name     = attr.value("service_name", "");
        dh.status           = attr.value("status", "Unknown");
        dh.checked_at       = attr.value("checked_at", "");
        dh.response_time_ms = attr.value("response_time_ms", 0);
        dh.message          = attr.value("message", "");
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
