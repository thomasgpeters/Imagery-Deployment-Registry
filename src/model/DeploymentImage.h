#pragma once

#include "Deployment.h"

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
        auto attr = j.contains("attributes") ? j["attributes"] : j;
        di.id            = jint(j, "id");
        di.deployment_id = jint(attr, "deployment_id");
        di.tier          = jstr(attr, "tier");
        di.service_name  = jstr(attr, "service_name");
        di.image_name    = jstr(attr, "image_name");
        di.tag           = jstr(attr, "tag", "latest");
        di.digest        = jstr(attr, "digest");
        di.registry_host = jstr(attr, "registry_host");
        di.registry_kind = jstr(attr, "registry_kind");
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
