#pragma once

#include "api/AlsClient.h"
#include "docker/DockerStatusProvider.h"
#include "model/Deployment.h"
#include "model/DeploymentEnvVar.h"
#include "model/DeploymentHealth.h"
#include "model/DeploymentImage.h"
#include "model/DeploymentLog.h"
#include "model/DeploymentPort.h"
#include "model/DeploymentTarget.h"

#include <Wt/WContainerWidget.h>
#include <Wt/WTabWidget.h>
#include <Wt/WTable.h>
#include <Wt/WText.h>

namespace dr {

class MainLayout;

/// Drill-down view for a single deployment.
///
/// Tabs:
///   Overview | Images | Ports | Compose | Env Vars | Health | Audit Log
class DeploymentDetailView : public Wt::WContainerWidget {
public:
    explicit DeploymentDetailView(api::AlsClient& client, MainLayout& layout);
    ~DeploymentDetailView() override;

    /// Fetch deployment by id and populate all tabs.
    void loadDeployment(int deploymentId);

private:
    void buildUI();

    // Tab content builders
    void populateOverview(const model::Deployment& d);
    void populateImages(const std::vector<model::DeploymentImage>& images);
    void populatePorts(const std::vector<model::DeploymentPort>& ports);
    void populateCompose(const std::string& composeContent);
    void populateEnvVars(const std::vector<model::DeploymentEnvVar>& vars);
    void populateHealth(const std::vector<model::DeploymentHealth>& checks);
    void populateAuditLog(const std::vector<model::DeploymentLog>& logs);
    void populateTargets(const std::vector<model::DeploymentTarget>& targets);
    void populateDockerStatus(const docker::ProjectStatus& status);

    /// Fetch live Docker status for a local compose deployment.
    void fetchDockerStatus(const std::string& projectName);

    // Fetch child resources for current deployment
    void fetchChildResources(int deploymentId);

    /// Delete the current deployment and all child resources, then navigate back.
    void deleteDeployment();

    /// Delete all child resources of a given type for a deployment.
    void deleteChildResources(const std::string& resource, int deploymentId,
                              std::shared_ptr<int> pending);

    api::AlsClient& client_;
    MainLayout& layout_;
    int currentDeploymentId_ = 0;

    /// Shared flag for preventing callbacks from touching a destroyed view.
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);

    Wt::WText*            title_          = nullptr;
    Wt::WContainerWidget* headerRow_      = nullptr;
    Wt::WText*            deleteStatus_   = nullptr;
    Wt::WTabWidget*       tabs_           = nullptr;

    // Tab containers
    Wt::WContainerWidget* overviewTab_  = nullptr;
    Wt::WContainerWidget* imagesTab_    = nullptr;
    Wt::WContainerWidget* portsTab_     = nullptr;
    Wt::WContainerWidget* composeTab_   = nullptr;
    Wt::WContainerWidget* envVarsTab_   = nullptr;
    Wt::WContainerWidget* healthTab_    = nullptr;
    Wt::WContainerWidget* auditLogTab_  = nullptr;
    Wt::WContainerWidget* targetsTab_   = nullptr;
    Wt::WContainerWidget* dockerTab_    = nullptr;
};

} // namespace dr
