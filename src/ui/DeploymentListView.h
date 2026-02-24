#pragma once

#include "api/AlsClient.h"
#include "model/Deployment.h"

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WText.h>

#include <vector>

namespace dr {

class MainLayout;

/// Table view showing all deployments with status badges and click-to-detail.
class DeploymentListView : public Wt::WContainerWidget {
public:
    DeploymentListView(api::AlsClient& client, MainLayout& layout);

    /// Fetch deployments from ALS and rebuild the table.
    void refresh();

private:
    void buildUI();
    void populateTable(const std::vector<model::Deployment>& deployments);

    /// Returns the Bootstrap badge class for a deployment status.
    static std::string statusBadgeClass(const std::string& status);

    api::AlsClient& client_;
    MainLayout& layout_;

    Wt::WText*  title_    = nullptr;
    Wt::WText*  status_   = nullptr;
    Wt::WTable* table_    = nullptr;
};

} // namespace dr
