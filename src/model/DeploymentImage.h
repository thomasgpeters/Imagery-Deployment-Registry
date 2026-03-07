#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace dr::model {

struct DeploymentImage {
    int         id              = 0;
    int         deployment_id   = 0;
    std::string tier;
    std::string service_name;
    std::string image_name;
    std::string tag             = "latest";
    std::string digest;
    std::string registry_host;
    std::string registry_kind;

    static DeploymentImage fromJson(const nlohmann::json& j) {
        DeploymentImage di;
        auto attr = j.value("attributes", j);
        if (j.contains("id")) {
            if (j["id"].is_number())
                di.id = j["id"].get<int>();
            else if (j["id"].is_string())
                try { di.id = std::stoi(j["id"].get<std::string>()); } catch (...) {}
        }
        di.deployment_id = attr.value("deployment_id", 0);
        di.tier          = attr.value("tier", "");
        di.service_name  = attr.value("service_name", "");
        di.image_name    = attr.value("image_name", "");
        di.tag           = attr.value("tag", "latest");
        di.digest        = attr.value("digest", "");
        di.registry_host = attr.value("registry_host", "");
        di.registry_kind = attr.value("registry_kind", "");
        return di;
    }

    nlohmann::json toJson() const {
        return {
            {"deployment_id", deployment_id},
            {"tier",          tier},
            {"service_name",  service_name},
            {"image_name",    image_name},
            {"tag",           tag},
            {"digest",        digest},
            {"registry_host", registry_host},
            {"registry_kind", registry_kind}
        };
    }

    /// Fully qualified image reference (e.g. "docker.io/postgres:16")
    std::string qualifiedName() const {
        std::string fqn;
        if (!registry_host.empty())
            fqn = registry_host + "/";
        fqn += image_name;
        if (!tag.empty())
            fqn += ":" + tag;
        return fqn;
    }
};

} // namespace dr::model
