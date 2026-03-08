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

/// Deployment overview — switchable between card-grid and table-list views.
class DeploymentListView : public Wt::WContainerWidget {
public:
    DeploymentListView(api::AlsClient& client, MainLayout& layout);
    ~DeploymentListView() override;

    /// Fetch deployments from ALS and rebuild both views.
    void reload();

    /// Start auto-refresh polling (default 15 seconds).
    void startPolling(int intervalSeconds = 15);
    /// Stop auto-refresh polling.
    void stopPolling();

private:
    void buildUI();
    void populateGrid(const std::vector<model::Deployment>& deployments);
    void populateTable(const std::vector<model::Deployment>& deployments);

    /// Toggle between grid and list views.
    void setViewMode(bool gridMode);

    static std::string statusBadgeClass(const std::string& status);
    static std::string targetIcon(const std::string& target, int size = 48);

    api::AlsClient& client_;
    MainLayout& layout_;

    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);

    Wt::WText*            title_     = nullptr;
    Wt::WText*            status_    = nullptr;
    Wt::WText*            gridBtn_   = nullptr;
    Wt::WText*            listBtn_   = nullptr;
    Wt::WContainerWidget* grid_      = nullptr;  // card grid
    Wt::WContainerWidget* listWrap_  = nullptr;  // table list wrapper
    Wt::WTable*           table_     = nullptr;
    std::unique_ptr<Wt::WTimer> pollTimer_;
    bool                  polling_   = false;
    bool                  gridMode_  = true;

    /// Cache of last-fetched deployments for view-mode switching.
    std::vector<model::Deployment> deployments_;
};

} // namespace dr
