#pragma once

#include "api/AlsClient.h"

#include <Wt/WContainerWidget.h>
#include <Wt/WMenu.h>
#include <Wt/WStackedWidget.h>
#include <Wt/WText.h>

#include <string>

namespace dr {

class DeploymentDetailView;
class DeploymentListView;

/// Top-level layout: sidebar navigation + stacked content area.
class MainLayout : public Wt::WContainerWidget {
public:
    MainLayout();

    /// Handle internal path changes (URL fragment routing).
    void handleInternalPath(const std::string& path);

    /// Shared ALS client.
    api::AlsClient& alsClient() { return alsClient_; }

    /// Navigate to the deployment detail view for a given deployment id.
    void showDeploymentDetail(int deploymentId);

private:
    void buildSidebar();
    void buildContent();

    api::AlsClient alsClient_{"http://localhost:5656/api"};

    Wt::WContainerWidget* sidebar_   = nullptr;
    Wt::WMenu*            menu_      = nullptr;
    Wt::WStackedWidget*   content_   = nullptr;
    Wt::WText*            brandTitle_ = nullptr;

    DeploymentListView*   listView_   = nullptr;
    DeploymentDetailView* detailView_ = nullptr;
};

} // namespace dr
