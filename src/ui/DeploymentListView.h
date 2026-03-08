#pragma once

#include "api/AlsClient.h"
#include "model/Deployment.h"

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WTimer.h>

#include <memory>
#include <vector>

namespace dr {

class MainLayout;

/// Table view showing all deployments with status badges and click-to-detail.
class DeploymentListView : public Wt::WContainerWidget {
public:
    DeploymentListView(api::AlsClient& client, MainLayout& layout);
    ~DeploymentListView() override;

    /// Fetch deployments from ALS and rebuild the table.
    void reload();

    /// Start auto-refresh polling (default 15 seconds).
    void startPolling(int intervalSeconds = 15);
    /// Stop auto-refresh polling.
    void stopPolling();

private:
    void buildUI();
    void populateTable(const std::vector<model::Deployment>& deployments);

    /// Returns the Bootstrap badge class for a deployment status.
    static std::string statusBadgeClass(const std::string& status);

    api::AlsClient& client_;
    MainLayout& layout_;

    /// Shared flag for preventing callbacks from touching a destroyed view.
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);

    Wt::WText*  title_    = nullptr;
    Wt::WText*  status_   = nullptr;
    Wt::WTable* table_    = nullptr;
    std::unique_ptr<Wt::WTimer> pollTimer_;
    bool        polling_   = false;
};

} // namespace dr
