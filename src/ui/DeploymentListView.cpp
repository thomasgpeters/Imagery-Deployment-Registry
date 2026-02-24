#include "ui/DeploymentListView.h"
#include "ui/MainLayout.h"

#include <Wt/WApplication.h>
#include <Wt/WText.h>

namespace dr {

DeploymentListView::DeploymentListView(api::AlsClient& client, MainLayout& layout)
    : client_(client), layout_(layout)
{
    buildUI();
}

void DeploymentListView::buildUI()
{
    setStyleClass("container-fluid");

    // Header row
    auto* header = addNew<Wt::WContainerWidget>();
    header->setStyleClass("d-flex justify-content-between align-items-center mb-4");

    title_ = header->addNew<Wt::WText>("<h4>Deployments</h4>");

    auto* refreshBtn = header->addNew<Wt::WText>(
        "<button class='btn btn-outline-primary btn-sm'>"
        "<i class='bi bi-arrow-clockwise me-1'></i>Refresh</button>");
    refreshBtn->clicked().connect([this] { refresh(); });

    // Status line
    status_ = addNew<Wt::WText>("");
    status_->setStyleClass("text-muted small mb-3");

    // Table
    table_ = addNew<Wt::WTable>();
    table_->setStyleClass("table table-hover table-striped align-middle");
    table_->setHeaderCount(1);

    // Header cells
    table_->elementAt(0, 0)->addNew<Wt::WText>("Environment");
    table_->elementAt(0, 1)->addNew<Wt::WText>("Stack");
    table_->elementAt(0, 2)->addNew<Wt::WText>("Status");
    table_->elementAt(0, 3)->addNew<Wt::WText>("Target");
    table_->elementAt(0, 4)->addNew<Wt::WText>("Version");
    table_->elementAt(0, 5)->addNew<Wt::WText>("Deployed By");
    table_->elementAt(0, 6)->addNew<Wt::WText>("Deployed At");

    for (int c = 0; c < 7; ++c)
        table_->elementAt(0, c)->setStyleClass("fw-bold");
}

void DeploymentListView::refresh()
{
    status_->setText("Loading...");

    client_.getAll("Deployment", [this](bool ok, const nlohmann::json& items) {
        if (!ok) {
            status_->setText("Failed to fetch deployments from ALS backend.");
            return;
        }

        std::vector<model::Deployment> deployments;
        for (const auto& item : items) {
            deployments.push_back(model::Deployment::fromJson(item));
        }

        status_->setText(std::to_string(deployments.size()) + " deployment(s)");
        populateTable(deployments);

        Wt::WApplication::instance()->triggerUpdate();
    });
}

void DeploymentListView::populateTable(const std::vector<model::Deployment>& deployments)
{
    // Clear data rows (keep header)
    while (table_->rowCount() > 1)
        table_->deleteRow(1);

    int row = 1;
    for (const auto& d : deployments) {
        table_->elementAt(row, 0)->addNew<Wt::WText>(d.environment_name);
        table_->elementAt(row, 1)->addNew<Wt::WText>(d.stack_name);

        auto* badge = table_->elementAt(row, 2)->addNew<Wt::WText>(d.status);
        badge->setStyleClass("badge " + statusBadgeClass(d.status));

        table_->elementAt(row, 3)->addNew<Wt::WText>(d.target + " / " + d.provider);
        table_->elementAt(row, 4)->addNew<Wt::WText>(d.version_label);
        table_->elementAt(row, 5)->addNew<Wt::WText>(d.deployed_by);
        table_->elementAt(row, 6)->addNew<Wt::WText>(d.deployed_at);

        // Click row → navigate to detail
        int deployId = d.id;
        table_->rowAt(row)->setStyleClass("cursor-pointer");
        table_->rowAt(row)->clicked().connect([this, deployId] {
            layout_.showDeploymentDetail(deployId);
        });

        ++row;
    }
}

std::string DeploymentListView::statusBadgeClass(const std::string& status)
{
    if (status == "Running")    return "bg-success";
    if (status == "Deploying")  return "bg-warning text-dark";
    if (status == "Failed")     return "bg-danger";
    if (status == "Stopped")    return "bg-secondary";
    if (status == "RolledBack") return "bg-info text-dark";
    if (status == "Pending")    return "bg-light text-dark";
    return "bg-secondary";
}

} // namespace dr
