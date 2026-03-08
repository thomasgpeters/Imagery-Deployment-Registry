#pragma once

#include "Deployment.h"

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
        auto attr = j.contains("attributes") ? j["attributes"] : j;
        dp.id             = jint(j, "id");
        dp.deployment_id  = jint(attr, "deployment_id");
        dp.service_name   = jstr(attr, "service_name");
        dp.container_port = jint(attr, "container_port");
        dp.host_port      = jint(attr, "host_port");
        dp.protocol       = jstr(attr, "protocol", "tcp");
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
