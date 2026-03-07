#pragma once

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
        auto attr = j.value("attributes", j);
        if (j.contains("id")) {
            if (j["id"].is_number())
                dl.id = j["id"].get<int>();
            else if (j["id"].is_string())
                try { dl.id = std::stoi(j["id"].get<std::string>()); } catch (...) {}
        }
        dl.deployment_id = attr.value("deployment_id", 0);
        dl.action        = attr.value("action", "");
        dl.status        = attr.value("status", "Started");
        dl.message       = attr.value("message", "");
        dl.created_at    = attr.value("created_at", "");
        dl.created_by    = attr.value("created_by", "");
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
