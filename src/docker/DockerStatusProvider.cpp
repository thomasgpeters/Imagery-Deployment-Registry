#include "docker/DockerStatusProvider.h"

#include <Wt/WLogger.h>

#include <array>
#include <cstdio>
#include <sstream>

namespace dr::docker {

// ---------------------------------------------------------------------------
// JSON serialization
// ---------------------------------------------------------------------------

nlohmann::json ContainerStatus::toJson() const
{
    return {
        {"container_id",   container_id},
        {"container_name", container_name},
        {"image",          image},
        {"state",          state},
        {"health",         health},
        {"status_text",    status_text},
        {"uptime_seconds", uptime_seconds}
    };
}

nlohmann::json ProjectStatus::toJson() const
{
    auto j = nlohmann::json{
        {"project_name",    project_name},
        {"overall_status",  overall_status}
    };
    auto arr = nlohmann::json::array();
    for (const auto& c : containers)
        arr.push_back(c.toJson());
    j["containers"] = arr;
    return j;
}

// ---------------------------------------------------------------------------
// Shell exec helper
// ---------------------------------------------------------------------------

std::string DockerStatusProvider::exec(const std::string& cmd)
{
    std::array<char, 4096> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
        result += buffer.data();

    pclose(pipe);
    return result;
}

// ---------------------------------------------------------------------------
// isAvailable
// ---------------------------------------------------------------------------

bool DockerStatusProvider::isAvailable()
{
    std::string out = exec("docker info --format '{{.ServerVersion}}' 2>/dev/null");
    return !out.empty() && out.find("Cannot connect") == std::string::npos;
}

// ---------------------------------------------------------------------------
// parseDockerPs — parse `docker ps --format json` (one JSON object per line)
// ---------------------------------------------------------------------------

std::vector<ContainerStatus> DockerStatusProvider::parseDockerPs(const std::string& output)
{
    std::vector<ContainerStatus> result;
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty() || line[0] != '{') continue;
        try {
            auto j = nlohmann::json::parse(line);

            ContainerStatus cs;
            cs.container_id   = j.value("ID", "");
            cs.container_name = j.value("Names", "");
            cs.image          = j.value("Image", "");
            cs.state          = j.value("State", "");
            cs.status_text    = j.value("Status", "");

            // Extract health from status text (e.g. "Up 3 hours (healthy)")
            auto pos = cs.status_text.find('(');
            if (pos != std::string::npos) {
                auto end = cs.status_text.find(')', pos);
                if (end != std::string::npos)
                    cs.health = cs.status_text.substr(pos + 1, end - pos - 1);
            }

            // Parse RunningFor if available
            std::string runFor = j.value("RunningFor", "");
            // Simple heuristic: look for hours/minutes/seconds
            if (!runFor.empty()) {
                // "About an hour", "3 hours", "5 minutes", "30 seconds"
                int64_t secs = 0;
                size_t numPos = runFor.find_first_of("0123456789");
                if (numPos != std::string::npos) {
                    int val = std::stoi(runFor.substr(numPos));
                    if (runFor.find("hour") != std::string::npos)
                        secs = val * 3600;
                    else if (runFor.find("minute") != std::string::npos)
                        secs = val * 60;
                    else if (runFor.find("second") != std::string::npos)
                        secs = val;
                    else if (runFor.find("day") != std::string::npos)
                        secs = val * 86400;
                }
                cs.uptime_seconds = secs;
            }

            result.push_back(std::move(cs));
        } catch (const std::exception& e) {
            Wt::log("warn") << "DockerStatusProvider: failed to parse line: " << e.what();
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// getAllContainers
// ---------------------------------------------------------------------------

std::vector<ContainerStatus> DockerStatusProvider::getAllContainers()
{
    std::string output = exec("docker ps -a --format json 2>/dev/null");
    return parseDockerPs(output);
}

// ---------------------------------------------------------------------------
// getProjectStatus
// ---------------------------------------------------------------------------

ProjectStatus DockerStatusProvider::getProjectStatus(const std::string& projectName)
{
    ProjectStatus ps;
    ps.project_name = projectName;

    if (projectName.empty()) {
        ps.overall_status = "Unknown";
        return ps;
    }

    // Filter by compose project label
    std::string cmd = "docker ps -a --filter 'label=com.docker.compose.project="
                      + projectName + "' --format json 2>/dev/null";
    std::string output = exec(cmd);
    ps.containers = parseDockerPs(output);

    if (ps.containers.empty()) {
        ps.overall_status = "Stopped";
        return ps;
    }

    // Determine overall status
    int running = 0, total = static_cast<int>(ps.containers.size());
    for (const auto& c : ps.containers) {
        if (c.state == "running") ++running;
    }

    if (running == total)
        ps.overall_status = "Running";
    else if (running == 0)
        ps.overall_status = "Stopped";
    else
        ps.overall_status = "Degraded";

    return ps;
}

} // namespace dr::docker
