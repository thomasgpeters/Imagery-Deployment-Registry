#pragma once

#include "Deployment.h"

#include <nlohmann/json.hpp>
#include <string>

namespace dr::model {

struct DeploymentLog {
    int         id              = 0;
    int         deployment_id   = 0;
    std::string action;
    std::string status          = "Started";
    std::string message;
    std::string created_at;
    std::string created_by;

    static DeploymentLog fromJson(const nlohmann::json& j) {
        DeploymentLog dl;
        auto attr = j.contains("attributes") ? j["attributes"] : j;
        dl.id            = jint(j, "id");
        dl.deployment_id = jint(attr, "deployment_id");
        dl.action        = jstr(attr, "action");
        dl.status        = jstr(attr, "status", "Started");
        dl.message       = jstr(attr, "message");
        dl.created_at    = jstr(attr, "created_at");
        dl.created_by    = jstr(attr, "created_by");
        return dl;
    }

    nlohmann::json toJson() const {
        return {
            {"deployment_id", deployment_id},
            {"action",        action},
            {"status",        status},
            {"message",       message},
            {"created_by",    created_by}
        };
    }
};

} // namespace dr::model
