#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace dr::docker {

/// Live status of a single Docker container.
struct ContainerStatus {
    std::string container_id;     // short 12-char ID
    std::string container_name;
    std::string image;
    std::string state;            // running, exited, paused, restarting, dead
    std::string health;           // healthy, unhealthy, starting, none, ""
    std::string status_text;      // human-readable e.g. "Up 3 hours (healthy)"
    int64_t     uptime_seconds = 0;

    nlohmann::json toJson() const;
};

/// Status summary for a Docker Compose project.
struct ProjectStatus {
    std::string project_name;
    std::string overall_status;   // Running, Degraded, Stopped, Unknown
    std::vector<ContainerStatus> containers;

    nlohmann::json toJson() const;
};

/// Queries the local Docker daemon via CLI for container status.
///
/// This is designed to run synchronously (blocking).  Call from a
/// background thread or Wt's WIOService to avoid blocking the UI.
class DockerStatusProvider {
public:
    /// Check if docker CLI is available.
    static bool isAvailable();

    /// Get status of all containers in a compose project.
    static ProjectStatus getProjectStatus(const std::string& projectName);

    /// Get status of all running containers.
    static std::vector<ContainerStatus> getAllContainers();

private:
    /// Execute a command and return stdout.
    static std::string exec(const std::string& cmd);

    /// Parse `docker ps --format json` output.
    static std::vector<ContainerStatus> parseDockerPs(const std::string& output);
};

} // namespace dr::docker
