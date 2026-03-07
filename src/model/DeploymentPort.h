#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace dr::model {

struct DeploymentPort {
    int         id              = 0;
    int         deployment_id   = 0;
    std::string service_name;
    int         container_port  = 0;
    int         host_port       = 0;
    std::string protocol        = "tcp";

    static DeploymentPort fromJson(const nlohmann::json& j) {
        DeploymentPort dp;
        auto attr = j.value("attributes", j);
        if (j.contains("id")) {
            if (j["id"].is_number())
                dp.id = j["id"].get<int>();
            else if (j["id"].is_string())
                try { dp.id = std::stoi(j["id"].get<std::string>()); } catch (...) {}
        }
        dp.deployment_id  = attr.value("deployment_id", 0);
        dp.service_name   = attr.value("service_name", "");
        dp.container_port = attr.value("container_port", 0);
        dp.host_port      = attr.value("host_port", 0);
        dp.protocol       = attr.value("protocol", "tcp");
        return dp;
    }

    nlohmann::json toJson() const {
        return {
            {"deployment_id",  deployment_id},
            {"service_name",   service_name},
            {"container_port", container_port},
            {"host_port",      host_port},
            {"protocol",       protocol}
        };
    }

    /// Human-readable mapping (e.g. "8080:80/tcp")
    std::string mappingString() const {
        return std::to_string(host_port) + ":" +
               std::to_string(container_port) + "/" + protocol;
    }
};

} // namespace dr::model
